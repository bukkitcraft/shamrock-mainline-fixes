# General Mobile GM 5 Plus (`shamrock`) Mainline Linux & postmarketOS Fixes

Bu depo, Qualcomm Snapdragon 617 (MSM8952) tabanlı **General Mobile GM 5 Plus** (`shamrock`) cihazında mainline Linux çekirdeği (6.12 / 7.1.x) ve postmarketOS (Phosh / Wayland) ortamı için geliştirilen çekirdek yamalarını, ses yönlendirme profillerini (ALSA UCM2), dinamik güç yönetimi servislerini ve donanım düzeltmelerini içerir.

---

## 1. Donanım Durumu Özeti

| Bileşen | Donanım / Yonga | Sürücü / Alt Sistem | Durum | Açıklama |
| :--- | :--- | :--- | :--- | :--- |
| **Ekran** | Dijin OTM1901A 1080x1920@60Hz DSI | `panel-dijing-otm1901a` | **Çalışıyor** | Sahte DSI backlight temizlendi, PMI8950 WLED'e bağlandı. |
| **Dokunmatik** | Goodix GT970 | `goodix` | **Çalışıyor** | I2C1 (BLSP1 QUP1) üzerinden tam çoklu dokunmatik. |
| **Wi-Fi** | Qualcomm WCN3660B (Pronto v3) | `wcn36xx` + `qcom_wcnss_pil` | **Çalışıyor** | BMPS haltACK düzeltildi; 270 Mbit/s, DNS, ping %0 kayıp. |
| **Hoparlör** | Awinic AW8738 Class-K Amp | `aw8738` + `aw8738-power-guard` | **Çalışıyor** | Mod 2 temiz boost; boştayken otomatik 0.1 µA derin uyku. |
| **Ahize** | PM8950 EAR PA (EAR_S) | `msm8916-wcd-analog` | **Çalışıyor** | BTL diferansiyel mod (`RDAC2 MUX: RX1`), net ses. |
| **Mikrofon** | PM8950 AMIC1 + Ext Bias 1 | `msm8916-wcd-analog` | **Çalışıyor** | +36 dB analog kazanç, 48 kHz kayıt test edildi. |
| **Titreşim** | PMI8950 Haptics Motor | `qcom_spmi_haptics` | **Çalışıyor** | `feedbackd` ve sistem bildirimleriyle tam senkronize. |
| **Modem** | Qualcomm Hexagon Q6V5 MSS | `qcom-q6v5-mss` | **Çalışıyor** | IPA devre dışı bırakılarak çökme döngüsü kökten engellendi. |
| **Termal / CPU**| Qualcomm TSENS v1 | `qcom_tsens` | **Çalışıyor** | Polling modunda, IRQ storm %0 CPU yükü ile çözüldü. |

---

## 2. Depo Yapısı

- `patches/`: Mainline çekirdek ağacına (`arch/arm64/boot/dts/`, `drivers/net/wireless/ath/wcn36xx/`, `sound/soc/`) uygulanan sıralı yamalar (`0001` - `0009`).
- `userspace/alsa-ucm/`: ALSA UCM2 ses konfigürasyon profili (`HiFi.conf`).
- `userspace/bin/`:
  - `aw8738_ctrl.c`: AW8738 amfisi için mikrosaniyelik Mod 2 darbeli sinyal üreteci.
  - `aw8738_power_guard.c`: ALSA PCM akışını (`/proc/asound/...`) anlık izleyip amfiyi boştayken uyutan hafif C nöbetçisi.
- `userspace/systemd/`:
  - `aw8738-power-guard.service`: Dinamik hoparlör uyku yönetimi servisi.
  - `shamrock-audio-init.service`: Açılışta ses mikser durumunu hazır eden servis.
- `userspace/modules-load.d/`: Çekirdek modüllerini açılışta otomatik yükleyen tanımlar.

---

## 3. Kritik Mühendislik Bulguları ve Çözümleri

### A. Wi-Fi (WCN3660B / Pronto v3) Çökme Engellemesi
MSM8952 üzerinde Pronto v3 donanımı, mainline `wcn36xx` sürücüsü BMPS (güç tasarrufu) moduna geçmeye çalıştığında firmware seviyesinde `halPMU haltACK` hatası verip kilitleniyordu. Sürücüde Pronto v3 için bu geçiş bypass edildi; Wi-Fi bağlantısı stabil 270 Mbit/s hıza ulaştı.

### B. Hoparlör Arka Plan Cızırtısı ve Dinamik Uyku
AW8738 amfisi tek telli (1-wire pulse) arayüz ile GPIO 38 üzerinden kontrol edilir. Varsayılan Mod 5 (5.2V aşırı kazanç) boştayken PM8950 DAC dip gürültüsünü yükseltiyordu.
1. Çözüm: Amfi Mod 2 (4.6V düşük gürültülü temiz boost) seviyesine ayarlandı.
2. `aw8738-power-guard` servisi ile ses bittiği milisaniyede GPIO 38 sıfırlanarak amfi 0.1 µA derin uykuya alındı. Dip gürültüsü tamamen sıfırlandı.

### C. Ahize (EAR_S) BTL Diferansiyel İptal Hatası
PM8950 ahize çıkışı dahili diferansiyel BTL amfiye bağlıdır. Girdi yönlendiricisi (`RDAC2 MUX`) yanlış yapılandırıldığında iki giriş aynı sinyali alıp birbirini yok ediyordu (common-mode rejection). `RDAC2 MUX` standart UCM değeri olan `RX1`'e alınarak ahize net ve temiz ses verir hale getirildi.

### D. Modem ve IPA BAM Çakışması
MSM8952, BAM tabanlı IPA-lite v2.6 kullandığı için mainline `drivers/net/ipa` sürücüsünün başlattığı QMI paketleri modem yazılımını `ipa_bam.c:726` assertion'ı ile çökertebiliyordu. Cihaz ağacında `&ipa { status = "disabled"; };` yapılarak modem çökmesiz, stabil hale getirildi.
