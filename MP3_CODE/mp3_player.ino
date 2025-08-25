#include "FS.h"
#include "SD_MMC.h"
#include "soc/soc.h"
#include "driver/rtc_io.h"
#include "soc/rtc_cntl_reg.h"

#include "AudioTools.h"
#include "AudioTools/AudioLibs/A2DPStream.h"

const char* bluetoothDeviceName = "Semih's AirPods Pro";

File file;
BluetoothA2DPSource source;

int songNDX = 0;
int playlistNDX= 0;
bool isPlaying = false;

long bytesRead = 0;

String getFileNameFromNDX(int ndx) {
  return "/" + String(playlistNDX) + "/" +  String(ndx) +".raw";
}
void changePlaylistToNDX(int ndx){
  playlistNDX = ndx;
  songNDX = 0;
  changeSongToNDX(songNDX);
}
void changeSongToNDX(int ndx) {
  if (file)
    file.close();

  delay(50);

  String playFile = getFileNameFromNDX(ndx);
  Serial.print("playing file: ");
  Serial.println(playFile);
  
  file = SD_MMC.open(playFile, FILE_READ);
  bytesRead = 0;
  
  if (!file) {
    Serial.println("failed to open file");
    isPlaying = false;
    
    songNDX = 0;
    changeSongToNDX(songNDX);
  }
}

int32_t getDataFrames(Frame* data, int32_t frameCount) {
  if (!isPlaying) {
    for (int i = 0; i < frameCount; i++) {
      data[i].channel1 = 0;
      data[i].channel2 = 0;
    }
    return frameCount;
  }

  if (!file) {
    for (int i = 0; i < frameCount; i++) {
      data[i].channel1 = 0;
      data[i].channel2 = 0;
    }
    return frameCount;
  }

  if (!file.available()) {
    songNDX++;
    changeSongToNDX(songNDX);
    return frameCount;
  }

  int32_t result = file.read((uint8_t*)data, frameCount * sizeof(Frame));
  delay(1);
  bytesRead ++;

  // Serial.printf("song NDX: %d | file: %d\n", songNDX, data);

  return frameCount;
}

void onTogglePlaying() {
  isPlaying = !isPlaying;
}
void onNextSong() {
  songNDX = songNDX + 1;
  changeSongToNDX(songNDX);
}
void onPreviousSong() {    
  if(songNDX == 0)
  {
    changeSongToNDX(songNDX);
    return;
  }
  if(bytesRead >= 800)
  {
    changeSongToNDX(songNDX);
    return;
  }

  songNDX = songNDX - 1;
  changeSongToNDX(songNDX);
}
void handleButtons(uint8_t id, bool isReleased) {
  if (isReleased) {
    Serial.print("button pressed: ");
    Serial.println(id);

    switch (id) {
      case 70:
        {
          onTogglePlaying();
        }
        break;
      case 75:
        {
          onNextSong();
        }
        break;
      case 76:
        {
          onPreviousSong();
        }
        break;
    }
  }
}

bool hasGreeted = false;
void startConnection() {
  source.set_auto_reconnect(true);
  source.set_data_callback_in_frames(getDataFrames);
  source.set_avrc_passthru_command_callback(handleButtons);

  source.start(bluetoothDeviceName);
  source.set_volume(127);

  hasGreeted = false;

  Serial.println("started bluetooth");
}
void stopConnection() {
  source.disconnect();
  source.end();

  hasGreeted = false;

  Serial.println("stopped bluetooth");
}
void connectionGreeting() {
  if (hasGreeted)
    return;

  changeSongToNDX(songNDX);

  hasGreeted = true;
  Serial.print("connection successful with: ");
  Serial.println(bluetoothDeviceName);
}

void setup() {
  Serial.begin(115200);
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Info);

  esp_sleep_enable_timer_wakeup(0);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);

  Serial.println("initializing SD card");
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD card initialization failed");
    return;
  }
  Serial.println("SD card initialized");

  startConnection();
}
void loop() {
  delay(1000);

  if (source.is_connected()) {
    connectionGreeting();
  }
}