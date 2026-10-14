# General Mobile GM 5 Plus (shamrock) — Mainline Linux 7.1.3 Fixes

Bu depo, **Qualcomm MSM8952** tabanlı **General Mobile GM 5 Plus (`shamrock`)** cihazında mainline Linux 7.1.3 çekirdeğini ve postmarketOS (Phosh) ortamını tam işlevsel çalıştırmak için geliştirilen hata düzeltmelerini, yama dosyalarını ve kullanıcı alanı yapılandırmalarını içerir.

Bu düzeltmeler **Kadir (`kadir1243/linux`)** ve mainline Qualcomm geliştirici topluluğuna katkı sağlamak amacıyla hazırlanmıştır.

---

## 📌 Çekirdek Dalı & Pull Request
- **Fork Deposu:** [https://github.com/bukkitcraft/linux](https://github.com/bukkitcraft/linux)
- **Düzeltme Dalı:** `shamrock-fixes`
- **GitHub Karşılaştırma / PR Bağlantısı:**  
  [https://github.com/bukkitcraft/linux/compare/msm89x7/7.1.3...shamrock-fixes](https://github.com/bukkitcraft/linux/compare/msm89x7/7.1.3...shamrock-fixes)

---

## 🛠️ Çözülen Sorunlar ve Teknik Açıklamalar

### 1. WCNSS (Pronto v3) PIL Boot Deadlock & Regülatör Yaşam Döngüsü
- **Dosya:** `drivers/remoteproc/qcom_wcnss.c`
- **Yama:** `patches/0001-remoteproc-qcom_wcnss-Fix-Pronto-v3-PIL-boot-deadloc.patch`
- **Sorun:** MSM8952 (Pronto v3) üzerinde Pronto firmware'i, `wcnss_ctrl` alt aygıtı SMD üzerinden NV ikilisini (binary) indirmeden önce `ready_irq` sinyali üretmez. Upstream kodun `wcnss_start()` içinde `ready_irq` beklemesi kilitlenmeye (timeout deadlock) neden oluyordu. Ayrıca upstream kod açılış fonksiyonunun sonunda Iris regülatörlerini kapatıyordu.
- **Çözüm:**
  - PAS kimlik doğrulama/reset sonrasında hemen dönülerek SMD kanalının açılması ve `wcnss_ctrl`'ün NV yüklemesini tamamlaması sağlandı.
  - Iris regülatörleri ve saatleri çalışma süresince açık bırakıldı, kapatma işlemi `wcnss_stop()` fonksiyonuna taşındı.
  - MSM8952'de bulunmayan SSCTL sysmon kaydı atlandı.

---

### 2. WCN36xx Firmware Çökmesi (halPMU haltACK) & Gereksiz NV Yüklemesi
- **Dosyalar:** `drivers/net/wireless/ath/wcn36xx/main.c`, `drivers/net/wireless/ath/wcn36xx/smd.c`
- **Yama:** `patches/0002-wifi-wcn36xx-Fix-Pronto-v3-firmware-freeze-and-skip-.patch`
- **Sorun:**
  1. NV parametreleri `qcom_wcnss_ctrl` tarafından zaten indirildiği için `wcn36xx_smd_load_nv()` ikinci kez SMD isteği (REQ 55) atıyor, 10 saniye zaman aşımına uğrayıp `rtnl_lock` kilidini kilitliyordu.
  2. Çip açıldıktan ~90ms sonra sürücünün `wcn36xx_smd_enter_imps()` (boşta güç tasarrufu) göndermesi üzerine Pronto firmware'i `halPMU.c: haltACK bit not set even after 5 millisecs` hatası vererek A2XB veriyolunu donduruyordu.
- **Çözüm:**
  - `wcn36xx_smd_load_nv()` çağrısı atlandı.
  - Pronto v3 için `wcn36xx_smd_enter_imps()` no-op yapılarak A2XB veriyolu kilitlenmesi engellendi.
  - Çip zaten çalışıyorsa (`is_started`) gereksiz restart ve çökmeyi önleyen koruma bayrağı eklendi.

---

### 3. Dijing OTM1901A Panel Sürücüsünde Ters Reset Kutuplaması (Ekranın Açılmaması)
- **Dosya:** `drivers/gpu/drm/panel/msm89x7-generated/panel-dijing-otm1901a.c`
- **Yama:** `patches/0003-drm-panel-panel-dijing-otm1901a-Fix-inverted-reset-G.patch`
- **Sorun:** DTS içinde `reset-gpios = <&tlmm 0 GPIO_ACTIVE_LOW>;` olarak tanımlıdır. Otomatik üretilen sürücüdeki `dijing_otm1901a_reset()` fonksiyonu sırasının sonunda `gpiod_set_value_cansleep(ctx->reset_gpio, 1)` çağırıyordu. `GPIO_ACTIVE_LOW` mantığında `1` değeri pini fiziksel olarak **LOW (0V / aktif reset)** seviyesine çeker. Bu nedenle panel açılmak istendiğinde sürekli donanımsal reset altında tutuluyor, LCD kontrolcüsü uyanamıyor ve arka ışık yanmıyordu. Kapatma (`unprepare`) esnasında ise `0` yazılarak pin serbest bırakılıyordu (kutuplama %100 tersti).
- **Çözüm:**
  - `prepare` aşamasında reset sinyali deassert edilerek pin fiziksel **HIGH (1.8V)** seviyesine çekildi.
  - `unprepare` aşamasında pin LOW yapılarak panel kapatıldı. Ekran ilk açılışta canlı görüntü vermeye başladı.

---

### 4. TSENS Seviye Kesme Fırtınası (Saniyede 42.000 Kesme / %100 CPU 0 Yükü)
- **Dosya:** `arch/arm64/boot/dts/qcom/msm8952.dtsi`
- **Yama:** `patches/0005-arm64-dts-qcom-msm8952-Eliminate-TSENS-unhandled-lev.patch`
- **Sorun:** MSM8952 üzerindeki `tsens-v0_1` sürücüsü `.set_trips` arayüzünü desteklemez. Eşlenmemiş sensör kanalları eşik değerini aştığında, seviye tetiklemeli (level-triggered) IRQ 184 (GIC 216) sürücü tarafından temizlenememektedir. Bu durum CPU 0 üzerinde saniyede **~42.000 kesme** üreten bir kesme fırtınasına (IRQ storm) yol açıyor, çekirdeği kilitliyor ve tüm sistemde aşırı kasmaya neden oluyordu.
- **Çözüm:**
  - DTS'teki `tsens` düğümünden seviye kesmesi (`interrupts = <GIC_SPI 184 ...>`) kaldırıldı.
  - Sıcaklık takibi kesme yükü olmaksızın periyodik yoklama (polling) ile sorunsuz ve soğuk şekilde devam etmektedir. Sistem genel boşta kalma oranı %94'e yükseldi.

---

### 5. Ses Kartı Makine Sürücüsünde Hoparlör Widget Eksikliği
- **Dosya:** `sound/soc/qcom/apq8016_sbc.c`
- **Yama:** `patches/0004-ASoC-qcom-apq8016_sbc-Add-Speaker-DAPM-widget-and-pi.patch`
- **Sorun:** `apq8016_sbc` sürücüsü DAPM widget listesinde yalnızca `Headphone Jack` ve `Mic Jack` tanımlıyordu. DTS'te harici hoparlör amfisi rotası eklendiğinde `devm_snd_soc_register_card()` kartı kaydedemiyor ve ses kartı tamamen düşüyordu.
- **Çözüm:**
  - Sürücüye `SND_SOC_DAPM_SPK("Speaker", NULL)` widget'ı ve `SOC_DAPM_PIN_SWITCH("Speaker")` mikser kontrolü eklendi.

---

### 6. WCN3660B 48 MHz Kristal & AW8738 Hoparlör Amfisi Entegrasyonu
- **Dosya:** `arch/arm64/boot/dts/qcom/msm8952-generalmobile-shamrock.dts`
- **Yama:** `patches/0006-arm64-dts-qcom-msm8952-shamrock-Fix-WCNSS-48MHz-XO-A.patch`
- **Düzeltmeler:**
  1. `wcnss_iris` uyumluluk dizesi GM 5 Plus donanımına uygun olarak `qcom,wcn3660b` (48 MHz XO) olarak güncellendi ve regülatörler bağlandı. Wi-Fi doğrudan ayağa kalktı.
  2. Cihazdaki ana hoparlör amfisi olan **Awinic AW8738** (`awinic,aw8738`, GPIO 38 üzerinde 1-wire puls mod 5) tanımlandı.
  3. Ses rotası Snapdragon telefonların standart tasarımına uygun olarak `HPH_R` -> `Speaker Amp IN` -> `Speaker Amp OUT` -> `Speaker` olarak bağlandı. Hem ahizeden hem de ana alt hoparlörden yüksek ve net ses çıkışı sağlandı.

---

## 📂 Kullanıcı Alanı Yapılandırmaları (Userspace)

1. **PulseAudio Hoparlör Tanımı (`userspace/pulseaudio/50-shamrock-audio.pa`):**
   - Phosh arayüzünde "Dummy Output" görünmesini engelleyerek donanımı doğrudan **AW8738** hoparlör çıkışı olarak bağlar:
     ```
     load-module module-alsa-sink device=hw:0,0 sink_name=speaker sink_properties=device.description=AW8738
     set-default-sink speaker
     ```
2. **ALSA UCM2 Profili (`userspace/alsa-ucm/HiFi.conf`):**
   - `gm-shamrock` ses kartı için `hw:0,0` oynatma ve `hw:0,1` kayıt eşleşmesini sağlar.
3. **TTL Ayar Betikleri (`userspace/bin/ttl64`, `ttl65`):**
   - Cihaz terminalinden mobil erişim noktası (tethering) TTL sınırını anında ayarlamak için yazılmıştır.

---

## 🚀 Yamaları Uygulama (Git AM)

Çekirdek ağacınızda bu yamaları doğrudan uygulamak için:

```bash
cd /path/to/linux
git checkout msm89x7/7.1.3
git am /path/to/patches/*.patch
```

---
*Hazırlayan:* **bukkitcraft** & **Jcode**
