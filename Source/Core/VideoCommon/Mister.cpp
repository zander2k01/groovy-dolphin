#include "Mister.h"

#ifdef _WIN32
#define GROOVYMISTER_WIN32_STATIC
#endif

#include "mister/groovymister_wrapper.h"
#include <Common/Logging/Log.h>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <Common/CommonTypes.h>

#include <fmt/format.h>

#include "Common/Config/Config.h"
#include "Core/Config/GraphicsSettings.h"
#include "VideoCommon/OnScreenDisplay.h"

namespace
{
// How long without any answer from the MiSTer before the connection counts as lost.
constexpr auto LOST_TIMEOUT = std::chrono::seconds(3);
// How often to try (re)connecting while the MiSTer doesn't answer.
constexpr auto RETRY_INTERVAL = std::chrono::seconds(2);

void ShowMessage(std::string message, u32 ms, u32 color)
{
  NOTICE_LOG_FMT(VIDEO, "GroovyMiSTer: {}", message);
  OSD::AddMessage(fmt::format("Groovy MiSTer: {}", message), ms, color);
}
}  // namespace

MiSTer::MiSTer() = default;

MiSTer::~MiSTer()
{
}

char* MiSTer::getPBufferBlit(void)
{
  return gmw_get_pBufferBlit(0);
}

char* MiSTer::getPBufferBlitDelta(void)
{
  return gmw_get_pBufferBlitDelta();
}


char* MiSTer::getPBufferAudio(void)
{
  return gmw_get_pBufferAudio();
}

void MiSTer::Close(void)
{
  // Don't free the connection while the emulation or audio thread is using it.
  std::lock_guard net_lock(m_net_mutex);
  std::lock_guard audio_lock(m_audio_mutex);
  if (m_connected)
    gmw_close();
  m_connected = false;
  m_lost = false;
  m_host.clear();
}

void MiSTer::Init(const char* mister_host, short mister_port, uint8_t lz4_frames,
                  uint32_t sound_rate, uint8_t sound_chan, uint8_t rgb_mode, uint16_t mister_mtu)
{
  std::lock_guard net_lock(m_net_mutex);
  std::lock_guard audio_lock(m_audio_mutex);

  // Reconnecting: close the old connection but keep the library object alive, since the input
  // thread may be polling it. Only close when it's open (it closes itself after a failed init).
  if (m_connected)
    gmw_reset();
  m_connected = false;

  const bool was_lost = m_lost;
  m_host = mister_host;
  m_last_attempt = std::chrono::steady_clock::now();

  if (m_host.empty())
  {
    ShowMessage("enter your MiSTer's IP address in Settings > Groovy MiSTer", 2500,
                OSD::Color::RED);
    return;
  }

  gmw_bindInputs(mister_host);
  if (gmw_init(mister_host, lz4_frames, sound_rate, sound_chan, rgb_mode, mister_mtu) < 0)
  {
    // No answer. Dolphin retries every few seconds (see ShouldRetry), so it's fine if the Groovy
    // core is started after the game.
    // Ask the MiSTer to start the Groovy core (needs the groovy_wake helper on the MiSTer, which
    // ignores repeats while the core is loading).
    const bool start_core = Config::Get(Config::GFX_GROOVY_START_CORE);
    if (start_core)
      gmw_wakeCore(mister_host);
    // Shown on every attempt, slightly longer than the retry interval, so it stays up while
    // retrying and disappears once connected.
    ShowMessage(start_core ? fmt::format("no answer from {}, starting the Groovy core on the "
                                         "MiSTer...",
                                         mister_host) :
                             fmt::format("no answer from {}. Is the Groovy core running on the "
                                         "MiSTer? Retrying...",
                                         mister_host),
                2500, OSD::Color::RED);
  }
  else
  {
    m_connected = true;
    m_lost = false;
    m_audio_warning_shown = false;
    m_pal_message_shown = false;
    ShowMessage(fmt::format("{} {}", was_lost ? "reconnected to" : "connected to", mister_host),
                OSD::Duration::NORMAL, OSD::Color::GREEN);
  }

  m_width_core = 0;
  m_height_core = 0;
  m_real_width = 0;
  m_real_height = 0;
  m_interlaced = 0;
  m_buffer_prog = 1;
  m_frame = 0;
  m_pal = false;
  m_samples = 0;
  m_mode = 9;
  m_match = 0;
  m_last_echo = 0;
  m_last_mister_frame = 0;
  m_last_echo_time = m_last_blit_time = std::chrono::steady_clock::now();
}

bool MiSTer::ShouldRetry() const
{
  return !m_connected && std::chrono::steady_clock::now() - m_last_attempt >= RETRY_INTERVAL;
}

void MiSTer::Switchres(int video_mode, int w, int h, double freq, bool progressive_fb)
{
  std::lock_guard net_lock(m_net_mutex);
  if (!m_connected)
    return;

  double px = 12.146841;
  uint16_t udp_hactive = 640;
  uint16_t udp_hbegin = 658;
  uint16_t udp_hend = 715;
  uint16_t udp_htotal = 772;
  uint16_t udp_vactive = 480;
  uint16_t udp_vbegin = 487;
  uint16_t udp_vend = 493;
  uint16_t udp_vtotal = 525;
  uint8_t udp_interlace = 1;
  m_pal = (freq > 55) ? false : true; 
  m_mode = video_mode;

  // 60 Hz games, or "NTSC Only" (which shows 50 Hz games at 60 Hz). 240p used to be in this list
  // too, which played 50 Hz games 20% too fast; they now get 288p at 50 Hz (below).
  if (freq > 55 || video_mode == 2)  // 15khz/ntsc/vga
  {
    if (w == 256)
    {
      px = 4.852622;
      udp_hactive = 256;
      udp_hbegin = 263;
      udp_hend = 286;
      udp_htotal = 309;
    }

    if (w == 320)
    {
      px = 6.046147;
      udp_hactive = 320;
      udp_hbegin = 329;
      udp_hend = 357;
      udp_htotal = 385;
    }

    if (w == 512)
    {
      px = 9.739500;
      udp_hactive = 512;
      udp_hbegin = 527;
      udp_hend = 573;
      udp_htotal = 619;
    }

    if (w == 608)
    {
      px = 11.533205;
      udp_hactive = 608;
      udp_hbegin = 625 ;
      udp_hend = 679;
      udp_htotal = 733;
    }

    if (w >= 656)
    {
      px = 12.445791;
      udp_hactive = 656;
      udp_hbegin = 675;
      udp_hend = 733;
      udp_htotal = 791;
    }

    if (w >= 660)
    {
      px = 12.540197;
      udp_hactive = 660;
      udp_hbegin = 679;
      udp_hend = 738;
      udp_htotal = 797;      
    }

    if (w >= 666)
    {
      px = 12.634602;
      udp_hactive = 666;
      udp_hbegin = 685;
      udp_hend = 744;
      udp_htotal = 803;
    }

    if (h == 224 && video_mode < 3)
    {
      udp_vactive = 224;
      udp_vbegin = 236;
      udp_vend = 239;
      udp_vtotal = 262;
      udp_interlace = 0;
    }

    if (h == 240 && video_mode < 3)  
    {      
      udp_vactive = 240;
      udp_vbegin = 244;
      udp_vend = 247;
      udp_vtotal = 262;
      udp_interlace = 0;
    }

    if (h >= 448 && video_mode == 1)  // 240p
    {      
      px = (w == 320) ? 06.046147 :
           (w == 512) ? 09.739500 :
           (w == 608) ? 11.533205 :
           (w == 656) ? 12.422085 :
           (w == 660) ? 12.516311 :
           (w == 666) ? 12.610536 :
                        12.123704;
      udp_vactive = 240;
      udp_vbegin = 244;
      udp_vend = 247;
      udp_vtotal = 262;
      udp_interlace = 0;
    }
    
    if (h == 448)
    {                       
      if (video_mode == 1)  // 240p
      {
        px = (w == 512) ? 09.739500 :
             (w == 608) ? 11.533205 :
             (w == 656) ? 12.422085 :
             (w == 660) ? 12.516311 :
             (w == 666) ? 12.610536 :
                          12.123704;
        udp_vactive = 224;
        udp_vbegin = 236;
        udp_vend = 239;
        udp_vtotal = 262;
        udp_interlace = 0;
      }    
      else 
      {
        udp_vactive = 448;
        udp_vbegin = 471;
        udp_vend = 477;  
      }
    }       
  }
  else //pal/vga
  {      
    px = 12.296875;      
    udp_hactive = 640;
    udp_hbegin = 658;
    udp_hend = 716;
    udp_htotal = 787;
    udp_vactive = 528;
    udp_vbegin = 558;
    udp_vend = 563;
    udp_vtotal = 625;

    if (w == 512)
    {
      px = 9.843750;
      udp_hactive = 512;
      udp_hbegin = 527;
      udp_hend = 573;
      udp_htotal = 630;
    }

    if (w == 608)
    {
      px = 11.703125;
      udp_hactive = 608;
      udp_hbegin = 626;
      udp_hend = 681;
      udp_htotal = 749;
    }

    if (w >= 650)
    {
      px = 12.5;
      udp_hactive = 650;
      udp_hbegin = 669;
      udp_hend = 728;
      udp_htotal = 800;
    }

    if (w >= 660)
    {
      px = 12.703125;
      udp_hactive = 660;
      udp_hbegin = 679;
      udp_hend = 739;
      udp_htotal = 813;
    }

    if (w >= 666)
    {
      px = 12.796875;
      udp_hactive = 666;
      udp_hbegin = 685;
      udp_hend = 745;
      udp_htotal = 819;
    }  

    if (h == 448)  
    {     
      udp_vactive = 448;
      udp_vbegin = 518;
      udp_vend = 523;
    }

    if (h >= 496)
    {
      udp_vactive = 496;
      udp_vbegin = 542;
      udp_vend = 547;
    }

    if (h >= 512)
    {
      udp_vactive = 512;
      udp_vbegin = 550;
      udp_vend = 555;
    }

    if (h >= 522)  
    {
      udp_vactive = 522;
      udp_vbegin = 555;
      udp_vend = 560;
    }

    if (h >= 528)
    {
      udp_vactive = 528;
      udp_vbegin = 558;
      udp_vend = 563;
    }    

    if (h >= 530) 
    {
      udp_vactive = 530;
      udp_vbegin = 559;
      udp_vend = 564;
    }

    if (h >= 538)
    {
      udp_vactive = 538;
      udp_vbegin = 563;
      udp_vend = 568;
    }

    if (h >= 574)
    {
      udp_vactive = 574;
      udp_vbegin = 581;
      udp_vend = 586;
    }

    if (video_mode == 1)  // PAL equivalent of 240p: 288p at 50 Hz
    {
      udp_vactive = 288;
      udp_vbegin = 290;
      udp_vend = 293;
      udp_vtotal = 312;
      udp_interlace = 0;
    }
  }

  // Real console picture size (fixes overscan). The timings above stretch every width across the
  // whole visible line, so a 640 wide picture is ~10% wider than on a real GameCube/Wii and its
  // edges end up in the CRT's overscan. Instead, use the timing the game programmed into the
  // video interface: the active picture and its position within a 13.5 MHz line.
  if (m_console_timing && video_mode != 3)
  {
    const int hlw = static_cast<int>(m_console_hlw);
    const int hsy = static_cast<int>(m_console_hsy);
    const int hbe = static_cast<int>(m_console_hbe);
    const int hbs = static_cast<int>(m_console_hbs);
    int active = hlw + hbs - hbe;
    int front_porch = hlw - hbs;
    const bool valid = hlw >= 400 && hlw <= 450 && active >= 600 && active <= 720 &&
                       front_porch > 0 && hsy > 0 && hbe > hsy;
    if (valid)
    {
      if (active & 1)
      {
        active--;
        front_porch++;
      }
      udp_hactive = static_cast<uint16_t>(active);
      udp_hbegin = static_cast<uint16_t>(active + front_porch);
      udp_hend = static_cast<uint16_t>(udp_hbegin + hsy);
      udp_htotal = static_cast<uint16_t>(2 * hlw);
      // Pick the pixel clock that gives exactly the game's refresh rate (13.5 MHz for standard
      // interlaced NTSC/PAL; slightly lower for 262-line 240p).
      const double lines_per_field = udp_interlace ? udp_vtotal / 2.0 : udp_vtotal;
      px = freq * udp_htotal * lines_per_field / 1000000.0;
    }
  }

  if (video_mode == 3)  // vga
  {
    px = px * 2;
    udp_interlace = 0;
    progressive_fb = 1;
  }
  m_interlaced = udp_interlace;
  m_frameField = 0;
  m_width_core = w;
  m_height_core = h;
  m_real_width = udp_hactive;
  m_real_height = udp_vactive;
  m_buffer_prog = (m_interlaced && !progressive_fb) ? 0 : 1;

  NOTICE_LOG_FMT(VIDEO, "GroovyMiSTer switchres applied {}x{}", udp_hactive, udp_vactive);

  // Tell the user once per connection when a game runs at 50 Hz, since many NTSC TVs can't show it.
  if (freq <= 55 && !m_pal_message_shown)
  {
    if (video_mode == 2)
    {
      ShowMessage("this game runs at 50 Hz (PAL). \"NTSC Only\" shows it at 60 Hz, so it plays "
                  "about 20% too fast. Use Auto NTSC/PAL, or the game's 60 Hz option if it has one.",
                  OSD::Duration::VERY_LONG, OSD::Color::YELLOW);
    }
    else if (video_mode != 3)
    {
      ShowMessage("this game runs at 50 Hz (PAL), so the MiSTer outputs 50 Hz. If your TV can't "
                  "show it, use the game's 60 Hz option if it has one.",
                  OSD::Duration::VERY_LONG, OSD::Color::YELLOW);
    }
    m_pal_message_shown = true;
  }                 

  gmw_switchres(px, udp_hactive, udp_hbegin, udp_hend, udp_htotal, udp_vactive, udp_vbegin,
                udp_vend, udp_vtotal, (progressive_fb && udp_interlace) ? 2 : udp_interlace);
}


void MiSTer::Blit(uint16_t vsync, uint8_t field)
{
  std::lock_guard net_lock(m_net_mutex);
  if (m_connected && m_real_width && m_real_height)
  {
    gmw_fpgaStatus status;
    gmw_getStatus(&status);
    m_frame++;         
 
    if (status.frame >= m_frame)
      m_frame = status.frame + 1;    

    //int matchDelta = m_buffer_prog ? m_match : 0;
    int matchDelta = 0;
    // Copy the frame (or one field of it) from m_texture_bytes into the send buffer. The presenter
    // always writes m_texture_bytes, so this copy is what keeps sending and drawing apart.
    {
      // The presenter writes m_texture_bytes on the GPU thread; don't read a half-written frame.
      std::lock_guard texture_lock(m_texture_mutex);

      uint8_t* tmp_buffer = (uint8_t*) gmw_get_pBufferBlit(field);
      uint8_t* tmp_buffer_delta = (uint8_t*) gmw_get_pBufferBlitDelta();
      const uint8_t* texture = reinterpret_cast<const uint8_t*>(m_texture_bytes);
      int dst_buff = 0;
      int src_buff = field ? m_real_width * 3 : 0;

      for (int y = 0; y < m_real_height; y++)
      {
        for (int x = 0; x < m_real_width; x++)
        {
          for (int c = 0; c < 3; c++)
          {
            // Read each byte once. tmp_buffer is our copy of what the MiSTer is showing, and the
            // delta sent must be exactly (new - tmp_buffer), or the two drift apart and the
            // difference stays on screen for good.
            const uint8_t current = texture[src_buff + c];
            if (m_delta_frames)
            {
              const uint8_t previous = tmp_buffer[dst_buff + c];
              if (current == previous)
              {
                matchDelta++;
                tmp_buffer_delta[dst_buff + c] = 0x00;
              }
              else
              {
                tmp_buffer_delta[dst_buff + c] = static_cast<uint8_t>(current - previous);
              }
            }
            tmp_buffer[dst_buff + c] = current;
          }
          dst_buff += 3;
          src_buff += 3;
        }
        if (!m_buffer_prog)
        {
          y++;
          src_buff += m_real_width * 3;
        }
      }
    }
    gmw_blit(m_frame, field, vsync, 15000, matchDelta);
    gmw_waitSync();
    CheckConnection();
  }
}

void MiSTer::CheckConnection()
{
  // Called after every blit, with m_net_mutex held. The MiSTer answers each blit, which updates
  // frameEcho; if that stops for a few seconds, the MiSTer is gone (core closed, cable, reboot).
  const auto now = std::chrono::steady_clock::now();
  gmw_fpgaStatus status;
  gmw_getStatus(&status);

  // Emulation was paused or loading: don't count that time as silence.
  if (now - m_last_blit_time > std::chrono::seconds(1))
    m_last_echo_time = now;
  m_last_blit_time = now;

  // The MiSTer's frame counter went backwards: the Groovy core was restarted. It needs a fresh
  // connection (video mode and all) before it shows anything again.
  if (status.frame + 60 < m_last_mister_frame)
  {
    ShowMessage("the Groovy core restarted, reconnecting...", OSD::Duration::NORMAL,
                OSD::Color::YELLOW);
    gmw_reset();
    m_connected = false;
    m_lost = true;
    m_last_mister_frame = 0;
    m_last_attempt = now - RETRY_INTERVAL;
    return;
  }
  m_last_mister_frame = status.frame;

  if (status.frameEcho != m_last_echo)
  {
    m_last_echo = status.frameEcho;
    m_last_echo_time = now;

    if (!m_audio_warning_shown && !status.audio && Config::Get(Config::GFX_GROOVY_AUDIO))
    {
      ShowMessage("audio is off in the Groovy core's menu on the MiSTer (Audio > Audio: On)",
                  OSD::Duration::VERY_LONG, OSD::Color::YELLOW);
      m_audio_warning_shown = true;
    }
  }
  else if (now - m_last_echo_time > LOST_TIMEOUT)
  {
    ShowMessage("lost connection to the MiSTer, reconnecting...", OSD::Duration::VERY_LONG,
                OSD::Color::RED);
    gmw_reset();
    m_connected = false;
    m_lost = true;
    // Retry right away on the next frame.
    m_last_attempt = now - RETRY_INTERVAL;
  }
}

void MiSTer::doSync(void)
{
  gmw_waitSync();
}

bool MiSTer::SetConsoleTiming(bool enabled, uint32_t half_line_width, uint32_t sync_width,
                              uint32_t blank_end, uint32_t blank_start)
{
  if (!enabled)
    half_line_width = sync_width = blank_end = blank_start = 0;

  const bool changed = enabled != m_console_timing || half_line_width != m_console_hlw ||
                       sync_width != m_console_hsy || blank_end != m_console_hbe ||
                       blank_start != m_console_hbs;
  m_console_timing = enabled;
  m_console_hlw = half_line_width;
  m_console_hsy = sync_width;
  m_console_hbe = blank_end;
  m_console_hbs = blank_start;
  return changed;
}

void MiSTer::QueueAudio(const void* samples, std::size_t num_bytes)
{
  std::lock_guard lock(m_audio_mutex);
  // The pending byte count is 16-bit. If sending stalls, drop audio instead of wrapping.
  if (!m_connected || m_samples + num_bytes > UINT16_MAX)
    return;

  char* buffer = getPBufferAudio();
  if (!buffer)
    return;

  memcpy(buffer + m_samples, samples, num_bytes);
  m_samples = static_cast<uint16_t>(m_samples + num_bytes);
}

void MiSTer::BlitAudio()
{
  std::lock_guard net_lock(m_net_mutex);
  std::lock_guard lock(m_audio_mutex);
  if (m_connected && m_real_width && m_real_height && m_samples)
    gmw_audio(m_samples);
  m_samples = 0;
}

void MiSTer::setDeltaFrames(uint8_t delta)
{
  m_delta_frames = delta;
}

void MiSTer::setMatch(uint32_t match)
{
  m_match = match;
}

uint8_t MiSTer::getField(void)
{
  uint8_t field = 0;
  if (!m_buffer_prog)
  {
    gmw_fpgaStatus status;
    gmw_getStatus(&status);    
    m_frameField = !status.vgaF1 ^ ((m_frame - status.frame) % 2);  // Calamity formula
    field = m_frameField;
  }
  return field;
}


bool MiSTer::isPAL(void)
{
  return m_pal;
}

bool MiSTer::isInterlaced(void)
{
  return (!m_buffer_prog);
}

bool MiSTer::isConnected(void)
{
  return m_connected;
}

uint16_t MiSTer::getWidth(void)
{
  return m_width_core;
}
uint16_t MiSTer::getHeight(void)
{
  return m_height_core;
}

uint16_t MiSTer::getRealWidth(void)
{
  return m_real_width;
}
uint16_t MiSTer::getRealHeight(void)
{
  return m_real_height;
}
uint8_t MiSTer::getMode(void)
{
  return m_mode;
}

MiSTer g_mister;

void MiSTer::UpdatePreview(const char* bgr, int width, int height)
{
  if (!bgr || width <= 0 || height <= 0)
    return;

  std::lock_guard lock(m_preview_mutex);
  const size_t size = static_cast<size_t>(width) * height * 3;
  m_preview.assign(reinterpret_cast<const uint8_t*>(bgr),
                   reinterpret_cast<const uint8_t*>(bgr) + size);
  m_preview_width = width;
  m_preview_height = height;
  m_preview_frame_id++;
}

bool MiSTer::GetPreview(std::vector<uint8_t>* out, int* width, int* height,
                        uint32_t* last_frame_id)
{
  std::lock_guard lock(m_preview_mutex);
  if (m_preview.empty() || m_preview_frame_id == *last_frame_id)
    return false;

  *out = m_preview;
  *width = m_preview_width;
  *height = m_preview_height;
  *last_frame_id = m_preview_frame_id;
  return true;
}
