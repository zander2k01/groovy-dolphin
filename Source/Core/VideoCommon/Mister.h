#include <inttypes.h>

#include <atomic>
#include <chrono>
#include <string>
#include <cstddef>
#include <mutex>
#include <vector>

class MiSTer
{
public:
  MiSTer();
  ~MiSTer();

  char* getPBufferBlit(void);
  char* getPBufferBlitDelta(void);
  char* getPBufferAudio(void);
  void Close(void);
  void Init(const char* mister_host, short mister_port, uint8_t lz4_frames, uint32_t sound_rate,
            uint8_t sound_chan, uint8_t rgb_mode, uint16_t mister_mtu);
  void Switchres(int video_mode, int w, int h, double freq, bool progressive_fb);
  // Sets the console's horizontal video timing (see VideoInterfaceManager::HorizontalTiming), used
  // by Switchres for 15 kHz modes so the picture is the same size as on a real console.
  // Pass enabled = false to use the original full-width timings. Returns true if it changed.
  bool SetConsoleTiming(bool enabled, uint32_t half_line_width, uint32_t sync_width,
                        uint32_t blank_end, uint32_t blank_start);
  void Blit(uint16_t vsync, uint8_t field);
  // Queues audio to send with the next frame. Called from the audio thread; BlitAudio is called
  // from the emulation thread, so both lock m_audio_mutex.
  void QueueAudio(const void* samples, std::size_t num_bytes);
  void BlitAudio();
  // True when not connected and it's time to try (re)connecting.
  bool ShouldRetry() const;
  void doSync(void);
  void setDeltaFrames(uint8_t delta);
  void setMatch(uint32_t match);

  uint8_t getField(void); 
  uint16_t getWidth(void);
  uint16_t getHeight(void);
  uint16_t getRealWidth(void);
  uint16_t getRealHeight(void);
  uint8_t getMode(void);
  
  bool isInterlaced(void);
  bool isConnected(void);
  bool isPAL(void);
  
  // Always progressive when outputs interlaced to alternate fields. Sized like the groovymister
  // send buffers (720x576), since PAL modes go up to 666x574 and console-size modes to 720 wide.
  char m_texture_bytes[720 * 576 * 3];
  // Guards m_texture_bytes: written by the presenter (GPU thread), read by Blit (CPU thread).
  std::mutex m_texture_mutex;

  // PC preview: a copy of the last frame sent to the MiSTer (BGR, 3 bytes per pixel), so it
  // can be shown in a window on the PC (e.g. for streaming on Discord).
  void UpdatePreview(const char* bgr, int width, int height);
  // Copies the preview frame into `out` if it changed since `last_frame_id`.
  // Returns false if there is no new frame.
  bool GetPreview(std::vector<uint8_t>* out, int* width, int* height, uint32_t* last_frame_id);

private: 
  uint32_t m_frame = 0;  
  uint8_t m_frameField = 0;
  uint8_t m_mode = 9; //0-ntsc/pal, 1-ntsc 240p, 2-ntsc, 3-vga
  uint16_t m_real_width = 0;
  uint16_t m_real_height = 0;
  uint16_t m_width_core = 0;
  uint16_t m_height_core = 0; 
  std::atomic<bool> m_connected = false;
  bool m_pal = false;  
  uint8_t m_interlaced = 0;
  uint8_t m_buffer_prog = 0;
  std::mutex m_audio_mutex;
  uint16_t m_samples = 0;
  uint8_t m_delta_frames = 0;
  uint8_t m_match = 0;

  // Lock order: m_net_mutex, then m_audio_mutex. m_net_mutex guards the connection (init, close,
  // switchres, blit), which is used from the GPU thread, the emulation thread and the audio path.
  std::mutex m_net_mutex;
  void CheckConnection();
  std::string m_host;
  bool m_lost = false;
  bool m_audio_warning_shown = false;
  bool m_pal_message_shown = false;
  uint32_t m_last_echo = 0;
  uint32_t m_last_mister_frame = 0;
  std::chrono::steady_clock::time_point m_last_attempt{};
  std::chrono::steady_clock::time_point m_last_echo_time{};
  std::chrono::steady_clock::time_point m_last_blit_time{};

  bool m_console_timing = false;
  uint32_t m_console_hlw = 0;
  uint32_t m_console_hsy = 0;
  uint32_t m_console_hbe = 0;
  uint32_t m_console_hbs = 0;

  std::mutex m_preview_mutex;
  std::vector<uint8_t> m_preview;
  int m_preview_width = 0;
  int m_preview_height = 0;
  uint32_t m_preview_frame_id = 0;
};

extern MiSTer g_mister;
