/**
 * This sketch grab's the robot's current state from our finite state machine,
 * and triggers different audio clips in response. 
 */
 
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <SFEMP3Shield.h>
#include <ros.h>
#include <std_msgs/Int16.h> // TO-DO: confirm which message type to import.
#define USB_CON

// Set up objects and vars for MP3 playback.
SdFat sd;
SFEMP3Shield MP3player;
int16_t last_ms_char;
int8_t buffer_pos;

// Set up ROS node handling & feedback channel.
ros::NodeHandle nh;

// Set up ROS subscribers.
ros::Subscriber<std_msgs::Int16> bot_state_sub("bot_state", &audioCb);

//------------------------------------------------------------------------------
/**
 * Bot state subscriber callback: Mapping the robot's state to the appropriate 
 * audio clip.
 * 
 * Index    Filename        State(str)     Action                   Audio Clip
 * -----    --------        ---------      ---------                ----------
 * 00001    TRACK004.mp3    0: within      playing elev music       Never Gonna Give You Up
 * 00002    TRACK001.mp3    1: asking      requesting aid @ elev    "Hello. Can you pls call elev for me?"
 * 00003    TRACK002.mp3    2: entering    entering elev            "Entering elevator; pls stand clear."
 * 00004    TRACK003.mp3    3: exiting     exiting elev             "Exiting elevator; pls stand clear."
 * 00005    TRACK005.mp3    0: within      playing elev music       generic elevator music
 * 00006    TRACK006.mp3    0: within      playing elev music       Scooby Doo theme song
 */
void audioCb( int std_msgs::Int16& state ) {

  char audio_index;
  int track_val;

  // Convert state to appropriate audio index.
  if (state == 1) audio_index = '00002';
  else if (state == 2) audio_index = '00003';
  else if (state == 3) audio_index = '00004';
  else if (state == 0) {
    track_val random(0,2);
    if (track_val == 0) audio_index = '00001';
    else if (track_val == 1) audio_index = '00005';
    else if (track_val == 2) audio_index = '00006';
  }

  parse_menu(audio_index);    // Send audio_index to parse_menu() to play appropriate track.

  delay(100);
  
}

uint32_t  millis_prv;


//------------------------------------------------------------------------------

/**
 * Initialize basic features, such as Serial port and MP3player objects with .begin.
 *
 * \note returned Error codes are typically passed up from MP3player, which in 
 * turns creates and initializes the SdCard objects.
*/

char buffer[6]; // 0-35K+null

void setup() {

  uint8_t result; //result code from some function as to be tested at later time.

  Serial.begin(115200);

  if(!sd.begin(SD_SEL, SPI_FULL_SPEED)) sd.initErrorHalt();  //Initialize the SdCard.
  if(!sd.chdir("/")) sd.errorHalt("sd.chdir");               // depending upon your SdCard environment, SPI_HAVE_SPEED may work better.

  result = MP3player.begin();  //Initialize the MP3 Player Shield

  last_ms_char = millis();  // stroke the inter character timeout.
  buffer_pos = 0;           // start the command string at zero length.

  nh.subscribe(bot_state_sub);

}

//------------------------------------------------------------------------------
/**
 * SPINNNYYY
 */
void loop() {
  nh.spinOnce();
  delay(1);
}

//------------------------------------------------------------------------------
/**
 * Parses through user's input to execute corresponding MP3player library functions, 
 * then displays a brief menu and prompts for next input command.
 */
void parse_menu(byte key_command) {

  uint8_t result; // result code from some function as to be tested at later time.

  //if s, stop the current track
  if(key_command == 's') {
    Serial.println(F("Stopping"));
    MP3player.stopTrack();

  //if 1-9, play corresponding track
  } else if(key_command >= '1' && key_command <= '9') {
    key_command = key_command - 48;   //convert ascii numbers to real numbers

#if USE_MULTIPLE_CARDS
    sd.chvol(); // assign desired sdcard's volume.
#endif
    //tell the MP3 Shield to play a track
    result = MP3player.playTrack(key_command);

    //check result, see readme for error codes.
    if(result != 0) {
      Serial.print(F("Error code: "));
      Serial.print(result);
      Serial.println(F(" when trying to play track"));
    } else {
      Serial.println(F("Playing:"));
    }

  //if +/- to change volume
  } else if((key_command == '-') || (key_command == '+')) {
    union twobyte mp3_vol; // create key_command existing variable that can be both word and double byte of left and right.
    mp3_vol.word = MP3player.getVolume(); // returns a double uint8_t of Left and Right packed into int16_t

    if(key_command == '-') {                              // note dB is negative; assume equal balance and use byte[1] for math
      if(mp3_vol.byte[1] >= 254) mp3_vol.byte[1] = 254;
      else mp3_vol.byte[1] += 2;                          // keep it simpler with whole dB's

    } else {
      if(mp3_vol.byte[1] <= 2) mp3_vol.byte[1] = 2;       // range check
      else mp3_vol.byte[1] -= 2;
    }
    
    // push byte[1] into both left and right assuming equal balance.
    MP3player.setVolume(mp3_vol.byte[1], mp3_vol.byte[1]); // commit new volume
    Serial.print(F("Volume changed to -"));
    Serial.print(mp3_vol.byte[1]>>1, 1);
    Serial.println(F("[dB]"));

  //if < or > to change Play Speed
  } else if((key_command == '>') || (key_command == '<')) {
    uint16_t playspeed = MP3player.getPlaySpeed(); // create key_command existing variable
    // note playspeed of Zero is equal to ONE, normal speed.
    if(key_command == '>') {                // note dB is negative; assume equal balance and use byte[1] for math
      if(playspeed >= 254) playspeed = 5;   // range check
      else playspeed += 1; // keep it simpler with whole dB's

    } else {
      if(playspeed == 0) playspeed = 0;  // range check
      else playspeed -= 1;
    }
    
    MP3player.setPlaySpeed(playspeed); // commit new playspeed
    Serial.print(F("playspeed to "));
    Serial.println(playspeed, DEC);

  /* Alterativly, you could call a track by it's file name by using playMP3(filename);
  But you must stick to 8.1 filenames, only 8 characters long, and 3 for the extension */
  } else if(key_command == 'f' || key_command == 'F') {
    uint32_t offset = 0;
    if (key_command == 'F') offset = 2000;

    //create a string with the filename
    char trackName[] = "track001.mp3";

#if USE_MULTIPLE_CARDS
    sd.chvol(); // assign desired sdcard's volume.
#endif
    //tell the MP3 Shield to play that file
    result = MP3player.playMP3(trackName, offset);   //check result, see readme for error codes.
    if(result != 0) {
      Serial.print(F("Error code: "));
      Serial.print(result);
      Serial.println(F(" when trying to play track"));
    }

  /* Display the file on the SdCard */
  } else if(key_command == 'd') {
    if(!MP3player.isPlaying()) {
      // prevent root.ls when playing, something locks the dump. but keeps playing.
      // yes, I have tried another unique instance with same results.
      // something about SdFat and its 500byte cache.
      Serial.println(F("Files found (name date time size):"));
      sd.ls(LS_R | LS_DATE | LS_SIZE);
    } else {
      Serial.println(F("Busy Playing Files, try again later."));
    }

  /* Get and Display the Audio Information */
  } else if(key_command == 'i') {
    MP3player.getAudioInfo();

  } else if(key_command == 'p') {
    if( MP3player.getState() == playback) {
      MP3player.pauseMusic();
      Serial.println(F("Pausing"));
    } else if( MP3player.getState() == paused_playback) {
      MP3player.resumeMusic();
      Serial.println(F("Resuming"));
    } else {
      Serial.println(F("Not Playing!"));
    }

  } else if(key_command == 't') {
    int8_t teststate = MP3player.enableTestSineWave(126);
    if(teststate == -1) {
      Serial.println(F("Un-Available while playing music or chip in reset."));
    } else if(teststate == 1) {
      Serial.println(F("Enabling Test Sine Wave"));
    } else if(teststate == 2) {
      MP3player.disableTestSineWave();
      Serial.println(F("Disabling Test Sine Wave"));
    }

  } else if(key_command == 'S') {
    Serial.println(F("Current State of VS10xx is."));
    Serial.print(F("isPlaying() = "));
    Serial.println(MP3player.isPlaying());

    Serial.print(F("getState() = "));
    switch (MP3player.getState()) {
    case uninitialized:
      Serial.print(F("uninitialized"));
      break;
    case initialized:
      Serial.print(F("initialized"));
      break;
    case deactivated:
      Serial.print(F("deactivated"));
      break;
    case loading:
      Serial.print(F("loading"));
      break;
    case ready:
      Serial.print(F("ready"));
      break;
    case playback:
      Serial.print(F("playback"));
      break;
    case paused_playback:
      Serial.print(F("paused_playback"));
      break;
    case testing_memory:
      Serial.print(F("testing_memory"));
      break;
    case testing_sinewave:
      Serial.print(F("testing_sinewave"));
      break;
    }
    Serial.println();

   } else if(key_command == 'b') {
    Serial.println(F("Playing Static MIDI file."));
    MP3player.SendSingleMIDInote();
    Serial.println(F("Ended Static MIDI file."));

#if !defined(__AVR_ATmega32U4__)
  } else if(key_command == 'm') {
      uint16_t teststate = MP3player.memoryTest();
    if(teststate == -1) {
      Serial.println(F("Un-Available while playing music or chip in reset."));
    } else if(teststate == 2) {
      teststate = MP3player.disableTestSineWave();
      Serial.println(F("Un-Available while Sine Wave Test"));
    } else {
      Serial.print(F("Memory Test Results = "));
      Serial.println(teststate, HEX);
      Serial.println(F("Result should be 0x83FF."));
      Serial.println(F("Reset is needed to recover to normal operation"));
    }

  } else if(key_command == 'e') {
    uint8_t earspeaker = MP3player.getEarSpeaker();
    if(earspeaker >= 3) earspeaker = 0;
    else earspeaker++;

    MP3player.setEarSpeaker(earspeaker); // commit new earspeaker
    Serial.print(F("earspeaker to "));
    Serial.println(earspeaker, DEC);

  } else if(key_command == 'r') {
    MP3player.resumeMusic(2000);

  } else if(key_command == 'R') {
    MP3player.stopTrack();
    MP3player.vs_init();
    Serial.println(F("Reseting VS10xx chip"));

  } else if(key_command == 'g') {
    int32_t offset_ms = 20000; // Note this is just an example, try your own number.
    Serial.print(F("jumping to "));
    Serial.print(offset_ms, DEC);
    Serial.println(F("[milliseconds]"));
    result = MP3player.skipTo(offset_ms);
    if(result != 0) {
      Serial.print(F("Error code: "));
      Serial.print(result);
      Serial.println(F(" when trying to skip track"));
    }

  } else if(key_command == 'k') {
    int32_t offset_ms = -1000; // Note this is just an example, try your own number.
    Serial.print(F("moving = "));
    Serial.print(offset_ms, DEC);
    Serial.println(F("[milliseconds]"));
    result = MP3player.skip(offset_ms);
    if(result != 0) {
      Serial.print(F("Error code: "));
      Serial.print(result);
      Serial.println(F(" when trying to skip track"));
    }

  } else if(key_command == 'O') {
    MP3player.end();
    Serial.println(F("VS10xx placed into low power reset mode."));

  } else if(key_command == 'o') {
    MP3player.begin();
    Serial.println(F("VS10xx restored from low power reset mode."));

  } else if(key_command == 'D') {
    uint16_t diff_state = MP3player.getDifferentialOutput();
    Serial.print(F("Differential Mode "));
    if(diff_state == 0) {
      MP3player.setDifferentialOutput(1);
      Serial.println(F("Enabled."));
    } else {
      MP3player.setDifferentialOutput(0);
      Serial.println(F("Disabled."));
    }

  } else if(key_command == 'V') {
    MP3player.setVUmeter(1);
    Serial.println(F("Use \"No line ending\""));
    Serial.print(F("VU meter = "));
    Serial.println(MP3player.getVUmeter());
    Serial.println(F("Hit Any key to stop."));

    while(!Serial.available()) {
      union twobyte vu;
      vu.word = MP3player.getVUlevel();
      Serial.print(F("VU: L = "));
      Serial.print(vu.byte[1]);
      Serial.print(F(" / R = "));
      Serial.print(vu.byte[0]);
      Serial.println(" dB");
      delay(1000);
    }
    Serial.read();

    MP3player.setVUmeter(0);
    Serial.print(F("VU meter = "));
    Serial.println(MP3player.getVUmeter());

  } else if(key_command == 'T') {
    uint16_t TrebleFrequency = MP3player.getTrebleFrequency();
    Serial.print(F("Former TrebleFrequency = "));
    Serial.println(TrebleFrequency, DEC);
    if (TrebleFrequency >= 15000) TrebleFrequency = 0;  // Range is from 0 - 1500Hz
    else TrebleFrequency += 1000;

    MP3player.setTrebleFrequency(TrebleFrequency);
    Serial.print(F("New TrebleFrequency = "));
    Serial.println(MP3player.getTrebleFrequency(), DEC);

  } else if(key_command == 'E') {
    int8_t TrebleAmplitude = MP3player.getTrebleAmplitude();
    Serial.print(F("Former TrebleAmplitude = "));
    Serial.println(TrebleAmplitude, DEC);
    if (TrebleAmplitude >= 7) TrebleAmplitude = -8; // Range is from -8 - 7dB
    else TrebleAmplitude++;

    MP3player.setTrebleAmplitude(TrebleAmplitude);
    Serial.print(F("New TrebleAmplitude = "));
    Serial.println(MP3player.getTrebleAmplitude(), DEC);

  } else if(key_command == 'B') {
    uint16_t BassFrequency = MP3player.getBassFrequency();
    Serial.print(F("Former BassFrequency = "));
    Serial.println(BassFrequency, DEC);
    if (BassFrequency >= 150) BassFrequency = 0;   // Range is from 20hz - 150hz
    else BassFrequency += 10;
    
    MP3player.setBassFrequency(BassFrequency);
    Serial.print(F("New BassFrequency = "));
    Serial.println(MP3player.getBassFrequency(), DEC);

  } else if(key_command == 'C') {
    uint16_t BassAmplitude = MP3player.getBassAmplitude();
    Serial.print(F("Former BassAmplitude = "));
    Serial.println(BassAmplitude, DEC);
    if (BassAmplitude >= 15) BassAmplitude = 0;  // Range is from 0 - 15dB
    else BassAmplitude++;

    MP3player.setBassAmplitude(BassAmplitude);
    Serial.print(F("New BassAmplitude = "));
    Serial.println(MP3player.getBassAmplitude(), DEC);

  } else if(key_command == 'M') {
    uint16_t monostate = MP3player.getMonoMode();
    Serial.print(F("Mono Mode "));
    if(monostate == 0) {
      MP3player.setMonoMode(1);
      Serial.println(F("Enabled."));
    } else {
      MP3player.setMonoMode(0);
      Serial.println(F("Disabled."));
    }
#endif

  /* List out music files on the SdCard */
  } else if(key_command == 'l') {
    if(!MP3player.isPlaying()) {
      Serial.println(F("Music Files found :"));
      SdFile file;
      char filename[13];
      sd.chdir("/",true);
      uint16_t count = 1;

      while (file.openNext(sd.vwd(),O_READ)) {
        file.getFilename(filename);
        if ( isFnMusic(filename) ) {
          Serial.print(F(": "));
          Serial.println(filename);
          count++;
        }
        file.close();
      }
      Serial.println(F("Enter Index of File to play"));

    } else {
      Serial.println(F("Busy Playing Files, try again later."));
    }
  } 

  // print prompt after key stroke has been processed.
  Serial.print(F("Enter s,1-9,+,-,>,<,f,F,d,i,p,t,S,b"));
#if !defined(__AVR_ATmega32U4__)
  Serial.print(F(",m,e,r,R,g,k,O,o,D,V,B,C,T,E,M:"));
#endif
  Serial.println(F(",l,h :"));
}

