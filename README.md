# RADIO

ESP32-WROOM-32E ve RDA5807M alıcı modülü ile geliştirilen FM radyo projesi.

**Sürüm:** `0.2.1`

Proje; FM frekans ayarı, otomatik istasyon arama, ses kontrolü, mute, bass,
mono/stereo seçimi, NVS tabanlı favoriler, durum LED'i ve ESP32 DAC üzerinden
açılış jingle'ı özelliklerini içerir.

## Donanım

- ESP32-WROOM-32E geliştirme kartı
- RDA5807M FM radyo modülü
- Ses yükselteci ve hoparlör
- 2 adet seek/ses düğmesi
- Durum LED'i
- FM anteni
- Jingle hattı için uygun ses katı

### Pin bağlantıları

| ESP32 pini | Bağlantı / görev |
| --- | --- |
| 3.3V | RDA5807M VCC |
| GND | RDA5807M, yükselteç ve diğer devrelerle ortak GND |
| GPIO21 | RDA5807M SDA |
| GPIO22 | RDA5807M SCL |
| GPIO25 | DAC1, jingle ses çıkışı |
| GPIO27 | Durum LED'i |
| GPIO13 | Seek up; basılı tutunca geçici olarak ses artırma |
| GPIO14 | Seek down; basılı tutunca geçici olarak ses azaltma |
| RDA5807M LOUT | Yükselteç ses girişi |
| RDA5807M ANT/FMIN | FM anteni |

GPIO13 ve GPIO14 girişleri `INPUT_PULLUP` olarak kullanılır; düğmeler pin ile
GND arasına bağlanmalıdır.

> RDA5807M beslemesi 3.3 V olmalıdır. ESP32 DAC çıkışı doğrudan hoparlör
> sürmek için uygun değildir; bir ses yükselteci veya uygun tampon katı gerekir.

## Yazılım özellikleri

- 87.0–108.0 MHz, 100 kHz kanal aralığı
- Yukarı/aşağı otomatik istasyon arama
- 0–15 arası ses seviyesi
- Mute, bass boost ve mono/stereo kontrolü
- RSSI okuma ve zayıf sinyal uyarısı
- Tune/seek işlemleri için bloklamayan durum makinesi
- I2C hata ve timeout raporlama
- ESP32 Preferences/NVS ile 6 favori yuvası altyapısı
- Radyo ses seviyesinden bağımsız, 16 kHz/8-bit mono PCM DAC jingle'ı
- Jingle sonrasında önceki mute ve ses seviyesi durumunu geri yükleme
- Bloklamayan LED yanıp sönme yönetimi

Favori altyapısı hazırdır ancak favori düğmesi ve seri port komutları henüz
tamamlanmamıştır.

## Kurulum ve yükleme

1. Arduino IDE'ye Espressif ESP32 kart paketini kurun.
2. Bu klasördeki `RADIO.ino` dosyasını Arduino IDE ile açın.
3. Kart olarak kullandığınız ESP32 geliştirme kartını seçin.
4. Kodu karta yükleyin.
5. Seri monitörü `115200 baud` ve satır sonu açık olacak şekilde başlatın.

Proje harici bir Arduino kütüphanesi gerektirmez. `Wire` ve `Preferences`,
ESP32 Arduino çekirdeği ile birlikte gelir.

## Seri port komutları

| Komut | Açıklama |
| --- | --- |
| `f101.1` | Frekansı 101.1 MHz olarak ayarlar |
| `v8` | Ses seviyesini 8 yapar |
| `m` veya `mute` | Mute durumunu değiştirir |
| `b` | Bass boost durumunu değiştirir |
| `n` | Mono/stereo durumunu değiştirir |
| `u` | Yukarı doğru istasyon arar |
| `d` | Aşağı doğru istasyon arar |
| `led on` | LED'i açar |
| `led off` | LED'i kapatır |
| `led` | LED durumunu değiştirir |
| `jingle` | DAC jingle'ını çalar |
| `radio reset` | RDA5807M'yi yeniden başlatır |
| `?` veya `status` | Frekans, ses, RSSI ve diğer durumları gösterir |
| `h` veya `help` | Yardım metnini gösterir |

## Düğmeler

- GPIO13 kısa basma: yukarı seek
- GPIO14 kısa basma: aşağı seek
- GPIO13 basılı tutma: ses artırma
- GPIO14 basılı tutma: ses azaltma

Basılı tutarak ses ayarı geçici bir kullanıcı arayüzüdür ve ileride ayrı
düğmeler veya encoder ile değiştirilebilir.

## Dosya yapısı

| Dosya | Görev |
| --- | --- |
| `RADIO.ino` | Başlatma ve ana döngü |
| `Data.h/.cpp` | Pinler, ayarlar ve ortak çalışma durumu |
| `Radio.h/.cpp` | RDA5807M I2C sürücüsü ve durum makinesi |
| `Buttons.h/.cpp` | Düğme debounce, seek ve ses kontrolü |
| `SerialCmd.h/.cpp` | Seri port komutları ve durum çıktıları |
| `Favorites.h/.cpp` | NVS tabanlı favori yönetimi |
| `Led.h/.cpp` | LED durumu ve yanıp sönme yönetimi |
| `Audio.h/.cpp` | DAC üzerinden PCM jingle çalma |
| `JingleData.h` | Flash'ta tutulan 16 kHz/8-bit mono PCM örnekleri |
| `YAPILACAKLAR.md` | Donanım ve yazılım geliştirme listesi |

## Ayarlar

Pinler ve kullanıcı tarafından değiştirilebilecek temel seçenekler `Data.h`
içindedir. Başlıca ayarlar:

- Varsayılan frekans ve ses
- FM bandı ve kanal aralığı
- I2C hızı ve timeout süreleri
- Jingle kazancı ve loud modu
- Açılışta jingle çalma
- LED polaritesi ve başlangıç durumu
- Düğme debounce ve uzun basma süreleri

## Mevcut durum

- Proje sürümü: `0.2.1`
- Son kayıtlı sürüm: `radio beta 1`
- Son kayıtlı kaynak commit'i: `3f94fec` (2026-09-24)
- Jingle öncesi geri dönüş noktası: `efedf83` (2026-08-21)

Bilinen geliştirme maddeleri ve öncelikler için [YAPILACAKLAR.md](YAPILACAKLAR.md)
dosyasına bakın.

Jingle verisini farklı bir kaynak dosyadan yeniden üretmek için proje kökünde:

```bash
python3 tools/generate_jingle.py /kaynak/ses.mp3
```

Komut, `tools/jingle_16k_u8.pcm` ve `JingleData.h` dosyalarını oluşturur.

## Önemli notlar

- Jingle ve RDA5807M ses çıkışlarını aynı yükselteç girişine doğrudan bağlamayın;
  uygun karıştırma, anahtarlama veya izolasyon devresi kullanın.
- Büyük donanım veya register değişikliklerinden önce çalışan sürümü commit ile
  yedekleyin.
- RDA5807M register sabitleri kullanılan modül revizyonuna göre doğrulanmalıdır.
