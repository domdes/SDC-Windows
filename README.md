# SDC-Windows (Yayasan Asyuhada Jaya Bekasi)

Aplikasi Desktop Voice Client Mumble / SDC (Surau Digital Communication) untuk sistem operasi Windows berbasis **Qt 6 (QML / C++)**, **Opus Codec**, dan **OpenSSL TLS**.

## Fitur Utama
- **Autentikasi Aman:** Login Google OAuth2 & integrasi backend Portal Asyuhada.
- **Audio Real-Time Rendah Latensi:** Enkoder/dekoder audio Opus berkinerja tinggi dengan noise gate dan studio limiter.
- **Mumble Protocol v1.4+:** Mendukung TLS terenkripsi, sinkronisasi pohon channel, mute/deafen status, dan Push-to-Talk (PTT).
- **Log Server Terintegrasi:** Panel log diagnostik di bagian bawah jendela utama untuk memantau aktivitas server Mumble secara real-time.
- **Dukungan URI Scheme:** `sdcyajb://` untuk auto-login dan integrasi portal web.

## Teknologi
- **Qt 6.11+ / C++17** (Qt Quick Controls, QML, Network, Multimedia)
- **CMake & Ninja Build System**
- **MinGW-w64 GCC 13.1+**
- **Opus Codec** (In-tree native compilation)
- **OpenSSL 3.x**
