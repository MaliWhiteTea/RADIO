# RADIO — Yapılacaklar / Notlar

**Son güncelleme:** 2026-09-24

## Sürüm noktaları

- **Mevcut proje sürümü:** `0.2.1`
- **Son kayıtlı beta:** `3f94fec` — `radio beta 1` (2026-09-24)
- **Jingle öncesi yedek:** `efedf83` — `Snapshot before PCM DAC jingle experiment.` (2026-08-21)

Geri dönüş gerekirse önce mevcut değişiklikleri commit et veya yedekle. Ardından
istenen commit'i incelemek için `git switch --detach <commit>` kullanılabilir.

## Yazılım geliştirme planı

Aşağıdaki işler önem sırasına göre düzenlenmiştir. Bir aşama tamamlanıp donanım
üzerinde doğrulanmadan sonraki büyük değişikliğe geçilmemelidir.

### 1. RDA5807M register ayarlarını doğrula

- [x] `REG05_BASE` değerini `0x8880` olarak düzelt. Ayrılmış bit 12'yi sıfırda
  bırak, `SEEKTH` istasyon arama eşiğini veri sayfasındaki varsayılan değer olan
  `8` yap ve LNA girişini `LNAP` olarak seç.
- [x] `REG04_VALUE` değerini `0x0A00` olarak düzelt. Ayrılmış bit 10'u sıfırda
  bırak, Avrupa için 50 µs de-emphasis ve soft-mute özelliklerini ayrı bit
  maskeleriyle etkinleştir.
- [ ] Yeni register değerlerini zayıf ve güçlü istasyonlarda dene. Seek işleminin
  gürültüyü istasyon olarak kabul etmediğini ve gerçek istasyonları atlamadığını
  doğrula. Bu madde gerçek RDA5807M, anten ve farklı sinyal seviyeleriyle donanım
  testi gerektirir.

### 2. Son kullanılan radyo ayarlarını NVS'ye kaydet

- [ ] Son başarıyla doğrulanan frekansı, ses seviyesini, mute durumunu, bass
  ayarını ve mono/stereo tercihini ESP32 `Preferences` alanında sakla.
- [ ] Frekansı kayan noktalı `float` değeri yerine 100 kHz kanal numarası olarak
  kaydet. Açılışta kanal numarasını tekrar MHz değerine dönüştür.
- [ ] Ses veya frekans her değiştiğinde flash belleğe yazma. Son kullanıcı
  işleminden sonra 2–5 saniye değişiklik olmazsa bütün ayarları bir defa kaydet.
- [ ] Kayıt yapmadan önce yeni değerin eski değerden farklı olduğunu kontrol et.
  Böylece gereksiz flash yazmaları önlenir.
- [ ] Kaydedilmiş veri yoksa veya kayıt geçersizse güvenli varsayılan ayarlarla
  başlat. Kayıt biçimi ileride değiştirilebilsin diye ayarlara bir sürüm numarası
  ekle.
- [ ] Kaydedilmiş frekansı yalnızca RDA5807M başarıyla başlatıldıktan sonra uygula.
  Tune işlemi başarısız olursa varsayılan frekansa geri dön ve geçersiz frekansı
  yeniden kaydetme.

### 3. Favori istasyon özelliğini tamamla

- [ ] Favori kaydetmeden önce radyonun hazır, işlemin boşta ve
  `frequencyValid` değerinin doğru olduğunu kontrol et. Doğrulanmamış bir
  frekansın favorilere yazılmasına izin verme.
- [ ] Seri porta `fav save 1`, `fav 1`, `fav list` ve `fav clear 1` komutlarını
  ekle. Hatalı yuva numarası veya boş favori için anlaşılır mesaj göster.
- [ ] Mevcut altı favori yuvasının tamamını seri komutlardan erişilebilir hale
  getir. Sabit `ACTIVE_SLOT = 0` kullanımına bağımlılığı kaldır.
- [ ] Favori verilerini MHz cinsinden `float` yerine 100 kHz kanal numarası ve
  geçerlilik bilgisiyle sakla.
- [ ] Favori düğmesi için boş bir GPIO belirle. Kısa basmada seçili favoriye git,
  uzun basmada mevcut frekansı aynı yuvaya kaydet.

### 4. Tune, seek ve I2C hatalarından sonra radyoyu toparla

- [ ] Tune veya seek zaman aşımında yalnızca yazılım durumunu `Idle` yapma.
  Donanımdaki TUNE/SEEK bitlerini temizle ve RDA5807M durum register'larını
  yeniden oku.
- [ ] Durum okuması birkaç kez arka arkaya başarısız olursa I2C hatasını ayrı
  olarak bildir. Gerekirse RDA5807M'ye soft reset uygulayıp son doğrulanmış
  frekansı yeniden ayarla.
- [ ] Otomatik toparlanma başarısız olursa radyoyu `Fault` durumuna geçir.
  Kullanıcıya seri porttan hata nedenini ve `radio reset` komutunu göster.
- [ ] Seek başarısız olduğunda veya tüm bant tarandığında daha önce doğrulanmış
  frekansı koru. Bilinmeyen bir donanım frekansını geçerli kabul etme.

### 5. Jingle oynatmayı tamamla ve doğrula

- [x] Orijinal stereo MP3 dosyasından 16 kHz, 8-bit mono PCM üret.
- [x] 16 kHz için 62 ve 63 µs aralıklarını dönüşümlü kullanarak ortalama örnek
  süresini 62,5 µs yap.
- [x] Jingle başlamadan önce radyo mute ve ses seviyesini sakla; jingle bitince
  aynı değerleri geri yükle.
- [ ] Yeni jingle'ı gerçek ESP32 ve yükselteç üzerinde dinle. Sesin doğru hızda
  çaldığını, başının kesilmediğini ve çalma sırasında reset oluşmadığını doğrula.
- [ ] `DAC_GAIN` ve `LOUD_MODE` ayarlarını dinleme testiyle karşılaştır. Sert
  kırpma duyuluyorsa `LOUD_MODE` özelliğini kapat ve gereken ses yüksekliğini
  yazılım kazancı yerine yükselteçten sağla.
- [ ] Jingle çalarken gelen mute, ses, seek, tune ve `radio reset` komutlarının
  davranışını belirle. İlk uygulamada bu komutları jingle bitene kadar reddetmek
  en güvenli seçenektir.
- [ ] Ana döngüdeki diğer işlemler duyulur çatlama oluşturursa DAC örneklerini
  I2S/DMA ile gönder. Mevcut oynatma kararlıysa bu değişikliği daha sonraya bırak.

### 6. Düğme ve LED davranışlarını düzenle

- [ ] Seek düğmelerine basılı tutarak ses ayarlama yönteminin geçici olduğunu
  kaldır. Ses için ayrı düğmeler veya rotary encoder kullanılacaksa pinleri ve
  kullanıcı davranışını belirle.
- [ ] LED durumlarını açık biçimde tanımla. Örneğin radyo hazırken sabit ışık,
  seek sırasında yavaş yanıp sönme, hata sırasında hızlı yanıp sönme ve favori
  kaydında üç kısa yanıp sönme kullanılabilir.
- [ ] Başlangıçta LED'i koşulsuz açan `START_ON = true` ayarını, tanımlanan durum
  göstergesi davranışına göre değiştir.

### 7. Derleme ve hata senaryolarını test et

- [ ] Kullanılan ESP32 kart modelini ve partition seçimini belgele. Arduino IDE
  veya `arduino-cli` ile temiz bir derleme yap ve flash/RAM kullanımını kaydet.
- [ ] RDA5807M bağlı değilken açılışı, çalışma sırasında I2C bağlantısının
  kesilmesini, tune/seek zaman aşımını ve boş favori çağrısını ayrı ayrı test et.
- [ ] İlk açılışta NVS boşken, geçerli ayarlar varken ve kayıt biçimi geçersizken
  doğru varsayılanların yüklendiğini doğrula.
- [ ] Jingle çalarken düğmelerin, seri komutların ve radyo durum makinesinin ana
  döngüyü kilitlemeden çalıştığını doğrula.
- [ ] Geliştirme tamamlandığında `DEBUG_RADIO = false` yap. Kullanıcı için gerekli
  hata mesajlarını korurken ayrıntılı register günlüklerini kapat.

### 8. Daha sonra eklenebilecek özellikler

- [ ] Frekans, ses, RSSI, stereo/mono, favori ve hata durumunu göstermek için bir
  OLED ekran ekle. Ekran güncellemesini ana döngüyü bloke etmeyecek şekilde yap.
- [ ] AUX girişi eklenecekse radyo, AUX ve jingle kaynakları arasında güvenli bir
  seçim yöntemi belirle. Birden fazla ses çıkışını doğrudan birbirine bağlama.
- [ ] Projenin README dosyasını her pin, komut veya bağlantı değişikliğinden sonra
  güncel tut.

## Donanım

- [ ] Transistör yükselteç (jingle / DAC GPIO25 → amp LIN)
  - NPN (BC547 / 2N2222)
  - GPIO25 --[10µF]--+-- Base
  - `[47k]-- 5V` / `[10k]-- GND`
  - 5V --[1k]-- Collector --[100µF]-- amp LIN
  - Emitter -- GND (ortak toprak)
  - Hoparlör sadece amp çıkışında
  - Sadece LIN (GPIO26 / RIN yok)
- [ ] Amp besleme / pot kontrolü (ses hâlâ kısıksa)
- [ ] RDA LOUT ile jingle hattının karışmaması (gerekirse anahtar / mute)

## Jingle (PCM + DAC)

- Kaynak: `/home/mert/Music/ILAC.mp3` (48 kHz, stereo, ~1.37 sn)
- Dosya: `JingleData.h` (16 kHz, 8-bit mono, ~1.37 sn)
- Üretim: `python3 tools/generate_jingle.py /home/mert/Music/ILAC.mp3`
- Pin: GPIO25 (DAC1) → (transistör) → amp LIN
- Seri komut: `jingle`
- Ayarlar: `Data.h` → `AudioCfg` (GAIN, LOUD_MODE, PLAY_ON_BOOT)

## Bağlantı özeti (hedef)

| ESP32 / RDA | Nereye |
|-------------|--------|
| 3.3V | RDA VCC |
| GND | RDA GND, amp GND, transistör GND |
| GPIO21/22 | RDA SDA/SCL |
| GPIO25 | [transistör] → amp LIN |
| GPIO27 | LED |
| GPIO13 | seek up (kısa) / vol+ (basılı tut, geçici) |
| GPIO14 | seek down (kısa) / vol- (basılı tut, geçici) |
| RDA LOUT | amp (jingle ile çakışmayı çöz) |
| RDA ANT | tel anten |

## Notlar

- ESP32 DAC tek başına zayıf; transistör veya iyi amp şart.
- Commit ile yedek almadan büyük deneme yapma.
