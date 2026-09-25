#include "Radio.h"
#include "Data.h"
#include <Wire.h>

/*
  RDA5807M akis (datasheet):
  Tune: TUNE=1 -> STC=1 (+ READCHAN eslesmesi) -> basari (TUNE HW tarafindan sifirlanir)
  Seek: SEEK=1 -> STC=1 -> SF? NoStation : commit READCHAN (SEEK HW sifirlanir)
  STC=0 beklemek YANLIS — islem bitince STC=1 kalabilir.
*/

enum class ChipPhase : uint8_t {
  Idle,
  SeekWaitStc,
  TuneWaitStc,
  Fault
};

static ChipPhase phase = ChipPhase::Fault;
static unsigned long phaseStartMs = 0;
static unsigned long nextPollMs = 0;

static RadioSeekEvent seekEvent = RadioSeekEvent::None;
static RadioTuneEvent tuneEvent = RadioTuneEvent::None;

static float pendingFreqMHz = 0;
static uint16_t pendingChannel = 0;
static bool hasPendingFreq = false;
static RadioInitError initError = RadioInitError::NotInitialized;
static bool radioReady = false;

static const char *phaseName(ChipPhase p) {
  switch (p) {
    case ChipPhase::Idle: return "Idle";
    case ChipPhase::SeekWaitStc: return "SeekWaitStc";
    case ChipPhase::TuneWaitStc: return "TuneWaitStc";
    case ChipPhase::Fault: return "Fault";
    default: return "?";
  }
}

static void dbgLine(const char *msg, bool statusOk, uint16_t reg0A) {
  if (!RadioCfg::DEBUG_RADIO) return;
  const bool stc = statusOk && ((reg0A & 0x4000) != 0);
  const bool sf = statusOk && ((reg0A & 0x2000) != 0);
  const uint16_t chan = statusOk ? (reg0A & 0x03FF) : 0;
  Serial.printf(
      "[RADIO] %s | phase=%s statusOk=%d STC=%d SF=%d reg0A=0x%04X READCHAN=0x%03X pending=%.1f(ch=%u) valid=%d\n",
      msg, phaseName(phase),
      statusOk ? 1 : 0, stc ? 1 : 0, sf ? 1 : 0, reg0A, chan,
      hasPendingFreq ? pendingFreqMHz : -1.0f,
      (unsigned)pendingChannel,
      g_radio.frequencyValid ? 1 : 0);
}

static void setPhase(ChipPhase next) {
  if (RadioCfg::DEBUG_RADIO && phase != next) {
    Serial.printf("[RADIO] %s -> %s\n", phaseName(phase), phaseName(next));
  }
  phase = next;
}

static bool rdaWriteSequential(const uint16_t *regs, size_t count) {
  Wire.beginTransmission(RadioCfg::I2C_ADDR_SEQ);
  for (size_t i = 0; i < count; i++) {
    Wire.write((uint8_t)(regs[i] >> 8));
    Wire.write((uint8_t)(regs[i] & 0xFF));
  }
  return Wire.endTransmission() == 0;
}

static bool rdaReadSequential(uint16_t *regs, size_t count) {
  const size_t bytes = count * 2;
  if (Wire.requestFrom((int)RadioCfg::I2C_ADDR_SEQ, (int)bytes) != (int)bytes) {
    return false;
  }
  for (size_t i = 0; i < count; i++) {
    uint8_t hi = Wire.read();
    uint8_t lo = Wire.read();
    regs[i] = ((uint16_t)hi << 8) | lo;
  }
  return true;
}

static bool rdaReadRandom(uint8_t reg, uint16_t *regs, size_t count) {
  Wire.beginTransmission(RadioCfg::I2C_ADDR_RAND);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;

  const size_t bytes = count * 2;
  if (Wire.requestFrom((int)RadioCfg::I2C_ADDR_RAND, (int)bytes) != (int)bytes) {
    return false;
  }
  for (size_t i = 0; i < count; i++) {
    uint8_t hi = Wire.read();
    uint8_t lo = Wire.read();
    regs[i] = ((uint16_t)hi << 8) | lo;
  }
  return true;
}

uint16_t radioFreqToChannel(float mhz) {
  if (mhz < RadioCfg::FREQ_MIN_MHZ) mhz = RadioCfg::FREQ_MIN_MHZ;
  if (mhz > RadioCfg::FREQ_MAX_MHZ) mhz = RadioCfg::FREQ_MAX_MHZ;
  return (uint16_t)((mhz - RadioCfg::FREQ_MIN_MHZ) / RadioCfg::FREQ_STEP_MHZ + 0.5f);
}

float radioChannelToFreq(uint16_t channel) {
  return RadioCfg::FREQ_MIN_MHZ + (channel * RadioCfg::FREQ_STEP_MHZ);
}

static uint16_t buildReg02() {
  uint16_t r = RadioCfg::REG02_BASE;
  if (g_radio.muted) r &= ~(1u << 14);
  else r |= (1u << 14);
  if (g_radio.forceMono) r |= (1u << 13);
  if (g_radio.bassOn) r |= (1u << 12);
  return r;
}

static uint16_t buildReg05() {
  return (uint16_t)(RadioCfg::REG05_BASE | (g_radio.volume & 0x0F));
}

static bool readStatus(uint16_t &reg0A, uint16_t &reg0B) {
  uint16_t st[2] = {0, 0};
  if (!rdaReadSequential(st, 2)) {
    if (!rdaReadRandom(0x0A, st, 2)) return false;
  }
  reg0A = st[0];
  reg0B = st[1];
  return true;
}

static bool writeRegsAt(float mhz, bool tuneBit, uint16_t reg02OrBits = 0) {
  const uint16_t chan = radioFreqToChannel(mhz);
  uint16_t reg03 = (uint16_t)(chan << 6);
  if (tuneBit) reg03 |= (1u << 4);
  uint16_t regs[4] = {
    (uint16_t)(buildReg02() | reg02OrBits),
    reg03,
    RadioCfg::REG04_VALUE,
    buildReg05()
  };
  return rdaWriteSequential(regs, 4);
}

static bool writeVerifiedRegs(bool tuneBit) {
  return writeRegsAt(g_radio.freqMHz, tuneBit, 0);
}

static void clearPending() {
  hasPendingFreq = false;
  pendingFreqMHz = 0;
  pendingChannel = 0;
}

static void markFreqUncertain() {
  g_radio.frequencyValid = false;
}

static void commitFreq(float mhz) {
  g_radio.freqMHz = radioChannelToFreq(radioFreqToChannel(mhz));
  g_radio.frequencyValid = true;
  clearPending();
}

static void enterFault(RadioInitError err) {
  clearPending();
  seekEvent = RadioSeekEvent::None;
  tuneEvent = RadioTuneEvent::None;
  markFreqUncertain();
  (void)writeVerifiedRegs(false);
  initError = err;
  radioReady = false;
  setPhase(ChipPhase::Fault);
}

static void finishOpToIdle() {
  clearPending();
  setPhase(ChipPhase::Idle);
}

bool radioIsReady() {
  return radioReady && phase != ChipPhase::Fault;
}

bool radioIsBusy() {
  return phase == ChipPhase::SeekWaitStc || phase == ChipPhase::TuneWaitStc;
}

bool radioIsSeeking() {
  return phase == ChipPhase::SeekWaitStc;
}

bool radioIsTuning() {
  return phase == ChipPhase::TuneWaitStc;
}

RadioInitError radioGetInitError() {
  return initError;
}

const char *radioInitErrorStr(RadioInitError err) {
  switch (err) {
    case RadioInitError::None: return "None";
    case RadioInitError::SoftResetI2c: return "SoftResetI2c";
    case RadioInitError::EnableI2c: return "EnableI2c";
    case RadioInitError::TuneStart: return "TuneStart";
    case RadioInitError::TuneStcTimeout: return "TuneStcTimeout";
    case RadioInitError::TuneChannelMismatch: return "TuneChannelMismatch";
    case RadioInitError::TuneI2cError: return "TuneI2cError";
    case RadioInitError::MissingTuneEvent: return "MissingTuneEvent";
    case RadioInitError::NotInitialized: return "NotInitialized";
    default: return "Unknown";
  }
}

bool radioApply(bool tunePulse) {
  if (!radioIsReady() || radioIsBusy()) return false;
  if (tunePulse) return radioSetFrequency(g_radio.freqMHz);
  return writeVerifiedRegs(false);
}

bool radioSetFrequency(float mhz) {
  if (!radioIsReady() || radioIsBusy()) return false;

  pendingChannel = radioFreqToChannel(mhz);
  pendingFreqMHz = radioChannelToFreq(pendingChannel);
  hasPendingFreq = true;

  if (!writeRegsAt(pendingFreqMHz, true, 0)) {
    clearPending();
    return false;
  }

  setPhase(ChipPhase::TuneWaitStc);
  phaseStartMs = millis();
  nextPollMs = millis() + RadioCfg::OPERATION_MIN_SETTLE_MS;
  return true;
}

bool radioSetVolume(uint8_t vol) {
  if (!radioIsReady() || radioIsBusy()) return false;
  if (vol > RadioCfg::VOLUME_MAX) vol = RadioCfg::VOLUME_MAX;
  const uint8_t old = g_radio.volume;
  g_radio.volume = vol;
  if (!writeVerifiedRegs(false)) {
    g_radio.volume = old;
    return false;
  }
  return true;
}

bool radioSetMute(bool mute) {
  if (!radioIsReady() || radioIsBusy()) return false;
  const bool old = g_radio.muted;
  g_radio.muted = mute;
  if (!writeVerifiedRegs(false)) {
    g_radio.muted = old;
    return false;
  }
  return true;
}

bool radioSetBass(bool on) {
  if (!radioIsReady() || radioIsBusy()) return false;
  const bool old = g_radio.bassOn;
  g_radio.bassOn = on;
  if (!writeVerifiedRegs(false)) {
    g_radio.bassOn = old;
    return false;
  }
  return true;
}

bool radioSetMono(bool on) {
  if (!radioIsReady() || radioIsBusy()) return false;
  const bool old = g_radio.forceMono;
  g_radio.forceMono = on;
  if (!writeVerifiedRegs(false)) {
    g_radio.forceMono = old;
    return false;
  }
  return true;
}

static bool waitIdleBlocking(unsigned long timeoutMs) {
  const unsigned long start = millis();
  while (radioIsBusy()) {
    radioUpdate();
    if ((millis() - start) >= timeoutMs) return false;
    delay(1);
  }
  return true;
}

bool radioInit() {
  radioReady = false;
  initError = RadioInitError::NotInitialized;
  clearPending();
  seekEvent = RadioSeekEvent::None;
  tuneEvent = RadioTuneEvent::None;
  setPhase(ChipPhase::Idle);

  Wire.begin(Pins::I2C_SDA_PIN, Pins::I2C_SCL_PIN);
  Wire.setClock(RadioCfg::I2C_HZ);

  if (RadioCfg::DEBUG_RADIO) {
    Serial.printf("[RADIO] Wire.begin SDA=%d SCL=%d\n",
                  Pins::I2C_SDA_PIN, Pins::I2C_SCL_PIN);
    Serial.printf("[RADIO] CFG REG02=0x%04X REG04=0x%04X REG05=0x%04X SEEKTH=%u\n",
                  (unsigned)RadioCfg::REG02_BASE,
                  (unsigned)RadioCfg::REG04_VALUE,
                  (unsigned)RadioCfg::REG05_BASE,
                  (unsigned)RadioCfg::SEEK_THRESHOLD);
  }

  uint16_t softReset[1] = { 0x0002 };
  if (!rdaWriteSequential(softReset, 1)) {
    Serial.println("[RADIO] Soft reset I2C yazma hatasi");
    enterFault(RadioInitError::SoftResetI2c);
    return false;
  }
  delay(RadioCfg::INIT_DELAY_MS);

  uint16_t enable[1] = { RadioCfg::REG02_BASE };
  if (!rdaWriteSequential(enable, 1)) {
    Serial.println("[RADIO] Enable I2C yazma hatasi");
    enterFault(RadioInitError::EnableI2c);
    return false;
  }
  delay(RadioCfg::INIT_DELAY_MS);

  g_radio.volume = RadioCfg::DEFAULT_VOLUME;
  g_radio.bassOn = RadioCfg::DEFAULT_BASS;
  g_radio.forceMono = RadioCfg::DEFAULT_MONO;
  g_radio.muted = RadioCfg::DEFAULT_MUTED;
  g_radio.freqMHz = RadioCfg::DEFAULT_FREQ_MHZ;
  g_radio.frequencyValid = RadioCfg::DEFAULT_FREQUENCY_VALID;

  radioReady = true;
  initError = RadioInitError::None;

  if (!radioSetFrequency(RadioCfg::DEFAULT_FREQ_MHZ)) {
    Serial.println("[RADIO] Tune baslatma hatasi");
    enterFault(RadioInitError::TuneStart);
    return false;
  }

  const unsigned long initWait =
      RadioCfg::TUNE_TIMEOUT_MS + RadioCfg::INIT_TUNE_WAIT_EXTRA_MS;

  if (!waitIdleBlocking(initWait)) {
    Serial.println("[RADIO] Init: TuneStcTimeout (STC/READCHAN gelmedi)");
    enterFault(RadioInitError::TuneStcTimeout);
    return false;
  }

  const RadioTuneEvent initEvent = radioConsumeTuneEvent();
  switch (initEvent) {
    case RadioTuneEvent::Ok:
      initError = RadioInitError::None;
      radioReady = true;
      setPhase(ChipPhase::Idle);
      Serial.println("[RADIO] Init tune OK");
      return true;
    case RadioTuneEvent::Timeout:
      Serial.println("[RADIO] Init tune: TuneStcTimeout");
      enterFault(RadioInitError::TuneStcTimeout);
      return false;
    case RadioTuneEvent::ChannelMismatch:
      Serial.println("[RADIO] Init tune: TuneChannelMismatch");
      enterFault(RadioInitError::TuneChannelMismatch);
      return false;
    case RadioTuneEvent::I2cError:
      Serial.println("[RADIO] Init tune: TuneI2cError");
      enterFault(RadioInitError::TuneI2cError);
      return false;
    case RadioTuneEvent::None:
    default:
      Serial.println("[RADIO] Init tune event eksik");
      enterFault(RadioInitError::MissingTuneEvent);
      return false;
  }
}

bool radioReadRssi(uint8_t &rssi) {
  uint16_t st[2] = {0, 0};
  if (!rdaReadSequential(st, 2)) {
    if (!rdaReadRandom(0x0A, st, 2)) return false;
  }
  rssi = (uint8_t)((st[1] >> 9) & 0x7F);
  return true;
}

bool radioSeekStart(bool up) {
  if (!radioIsReady() || radioIsBusy()) return false;

  clearPending();

  uint16_t seekBits = (1u << 8);
  if (up) seekBits |= (1u << 9);

  if (!writeRegsAt(g_radio.freqMHz, false, seekBits)) return false;

  setPhase(ChipPhase::SeekWaitStc);
  phaseStartMs = millis();
  nextPollMs = millis() + RadioCfg::OPERATION_MIN_SETTLE_MS;
  return true;
}

RadioSeekEvent radioConsumeSeekEvent() {
  const RadioSeekEvent ev = seekEvent;
  seekEvent = RadioSeekEvent::None;
  return ev;
}

RadioTuneEvent radioConsumeTuneEvent() {
  const RadioTuneEvent ev = tuneEvent;
  tuneEvent = RadioTuneEvent::None;
  return ev;
}

void radioUpdate() {
  if (phase != ChipPhase::SeekWaitStc && phase != ChipPhase::TuneWaitStc) return;

  const unsigned long now = millis();
  if ((long)(now - nextPollMs) < 0) return;
  nextPollMs = now + RadioCfg::STATUS_POLL_MS;

  uint16_t reg0A = 0, reg0B = 0;
  const bool haveStatus = readStatus(reg0A, reg0B);
  if (!haveStatus) {
    dbgLine("status I2C fail", false, 0);
    // Tek poll fail: timeout'a birak; surekli fail timeout'ta I2cError
    if (phase == ChipPhase::TuneWaitStc &&
        (now - phaseStartMs) >= RadioCfg::TUNE_TIMEOUT_MS) {
      clearPending();
      markFreqUncertain();
      tuneEvent = RadioTuneEvent::I2cError;
      finishOpToIdle();
    } else if (phase == ChipPhase::SeekWaitStc &&
               (now - phaseStartMs) >= RadioCfg::SEEK_TIMEOUT_MS) {
      clearPending();
      markFreqUncertain();
      seekEvent = RadioSeekEvent::Timeout;
      finishOpToIdle();
    }
    return;
  }

  const bool stc = (reg0A & 0x4000) != 0;
  const bool sf = (reg0A & 0x2000) != 0;
  const uint16_t readChan = reg0A & 0x03FF;

  switch (phase) {
    case ChipPhase::TuneWaitStc: {
      if (stc) {
        dbgLine("tune STC=1", true, reg0A);
        // Eski STC riski: READCHAN hedef ile eslesmeli
        if (readChan != pendingChannel) {
          dbgLine("tune READCHAN mismatch (eski STC?), bekleniyor", true, reg0A);
          if ((now - phaseStartMs) >= RadioCfg::TUNE_TIMEOUT_MS) {
            clearPending();
            markFreqUncertain();
            tuneEvent = RadioTuneEvent::ChannelMismatch;
            finishOpToIdle();
          }
          break;
        }
        if (sf) {
          // Tune icin SF beklenmez; yine de kanal eslesiyorsa kabul et
          dbgLine("tune SF=1 ama kanal eslesiyor", true, reg0A);
        }

        // Opsiyonel TUNE=0 yazisi (basari STC/READCHAN'a bagli; STC=0 beklenmez)
        if (!writeRegsAt(pendingFreqMHz, false, 0)) {
          Serial.println("[RADIO] Uyari: TUNE=0 I2C yazma hatasi (STC/READCHAN OK)");
        }

        commitFreq(pendingFreqMHz);
        tuneEvent = RadioTuneEvent::Ok;
        setPhase(ChipPhase::Idle);
        dbgLine("tune OK commit", true, reg0A);
      } else if ((now - phaseStartMs) >= RadioCfg::TUNE_TIMEOUT_MS) {
        dbgLine("tune STC timeout", true, reg0A);
        clearPending();
        markFreqUncertain();
        tuneEvent = RadioTuneEvent::Timeout;
        finishOpToIdle();
      }
      break;
    }

    case ChipPhase::SeekWaitStc: {
      if (stc) {
        dbgLine("seek STC=1", true, reg0A);
        if (sf) {
          clearPending();
          seekEvent = RadioSeekEvent::NoStation;
          setPhase(ChipPhase::Idle);
          dbgLine("seek NoStation", true, reg0A);
        } else {
          const float found = radioChannelToFreq(readChan);
          commitFreq(found);
          seekEvent = RadioSeekEvent::Ok;
          setPhase(ChipPhase::Idle);
          dbgLine("seek OK commit", true, reg0A);
        }
      } else if ((now - phaseStartMs) >= RadioCfg::SEEK_TIMEOUT_MS) {
        dbgLine("seek STC timeout", true, reg0A);
        clearPending();
        markFreqUncertain();
        seekEvent = RadioSeekEvent::Timeout;
        finishOpToIdle();
      }
      break;
    }

    default:
      break;
  }
}
