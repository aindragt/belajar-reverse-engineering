# Belajar Reverse Engineering: Bypass Cek Token dengan x64dbg

Latihan dasar reverse engineering di Windows (x86-64). Kita membuat program kecil yang meminta token, lalu memakai **x64dbg** untuk mengubah alur logikanya supaya token apa pun diterima. Caranya cukup dengan **mengubah satu byte** di file `.exe`.

> ⚠️ Untuk tujuan edukasi saja. Praktikkan hanya pada program milik sendiri.

## Isi Repository

| File | Keterangan |
|------|------------|
| `aplikasi.cpp` | Source code program pengecek token |
| `aplikasi.exe` | Hasil compile asli (hanya menerima token yang benar) |
| `cracked.exe`  | Versi yang sudah di-patch lewat x64dbg (menerima token apa pun) |

## 1. Program Target

```cpp
if (token == "indra-pramudya-123") {
    cout << "Selamat menikmati aplikasi kami!" << endl;
} else {
    cout << "Token ditolak!" << endl;
}
```

Compile (MSYS2 / MinGW-w64, g++):

```bash
g++ aplikasi.cpp -o aplikasi.exe
```

## 2. Patch dengan x64dbg

1. Buka `aplikasi.exe` di **x64dbg** (versi 64-bit), lalu tekan `F9` sampai program berhenti di entry point/berjalan.
2. Cari string: klik kanan di jendela CPU → **Search for → Current Module → String references**.
   Cari `"Token ditolak!"` atau `"Selamat menikmati aplikasi kami!"`, lalu double-click untuk lompat ke kodenya.
3. Sedikit di atasnya akan terlihat pola pengecekan token:

   ```asm
   call  operator==(string, const char*)  ; bandingkan input dengan token asli
   test  al, al                           ; hasil == 0 (token salah)?
   je    <alamat "Token ditolak!">        ; 74 2A -> kalau salah, lompat ke cabang gagal
   lea   rdx, ["Selamat menikmati..."]    ; kalau benar, lanjut ke cabang sukses
   ```

4. Pilih baris `je`, tekan `Space` (Assemble), lalu ubah tujuan lompatan ke **alamat instruksi tepat di bawahnya** (baris `lea rdx, ...`). Byte-nya berubah dari `74 2A` menjadi `74 00`.
5. Simpan: **File → Patch file** (`Ctrl+P`) → **Patch File** → simpan sebagai `cracked.exe`.

> 💡 x64dbg biasanya menampilkan alamat seperti `00007FF6xxxx152A` karena ASLR, jadi base address-nya bisa berbeda setiap kali dijalankan. Yang tetap sama adalah **RVA `0x152A`** (offset dari base image).

## 3. Kenapa Ini Berhasil?

- `operator==` mengembalikan `1` (benar) atau `0` (salah) di register `al`.
- `test al, al` + `je` artinya: **jika hasilnya 0, lompat** ke cabang "Token ditolak!".
- `74 XX` = `je` dengan jarak lompat `XX` byte dari instruksi berikutnya. Jika `XX = 00`, tujuan lompatannya adalah instruksi berikutnya itu sendiri. Jadi **lompat atau tidak, hasilnya sama**: program masuk ke cabang sukses.

| | Byte | Instruksi |
|---|---|---|
| Sebelum | `74 2A` | `je +0x2A` → ke "Token ditolak!" |
| Sesudah | `74 00` | `je +0x00` → ke instruksi berikutnya (sukses) |

Verifikasi hanya 1 byte yang berubah:

```bash
cmp -l aplikasi.exe cracked.exe
# 2348  52   0   -> file offset 0x92B (posisi 1-based, nilai oktal: 052 = 0x2A -> 0x00)
```

Menghitung file offset dari RVA (lihat `objdump -h aplikasi.exe`):

```
file_offset = RVA - .text VMA(RVA) + .text FileOffset
            = 0x152B - 0x1000 + 0x400
            = 0x92B
```

Tanpa x64dbg, patch yang sama bisa dilakukan dengan hex editor (HxD, dll.) atau lewat terminal:

```bash
cp aplikasi.exe cracked.exe
printf '\x00' | dd of=cracked.exe bs=1 seek=$((0x92B)) conv=notrunc
```

## 4. Hasil

```
> aplikasi.exe
Masukkan token: test
Token ditolak!

> cracked.exe
Masukkan token: test
Selamat menikmati aplikasi kami!
```

## Alternatif Patch Lain

- Ganti `je` dengan dua `nop` (`90 90`): klik kanan → **Binary → Fill with NOPs**. Lompatannya hilang sama sekali.
- Ganti `je` (`74`) dengan `jne` (`75`): logikanya terbalik, token salah diterima tapi token benar justru ditolak.
- Tanpa patch file: pasang breakpoint (`F2`) di `je`, lalu saat berhenti ubah flag **ZF** menjadi `0` di panel register. Ini hanya berlaku selama sesi debugging.

## Pelajaran

- Pengecekan `if` di source code berubah menjadi pasangan `cmp/test` + **conditional jump** di assembly. Itulah target utama saat cracking sederhana.
- String references adalah titik awal tercepat untuk menemukan logika penting di dalam program.
- Validasi yang sepenuhnya dilakukan di sisi klien bisa di-bypass dengan mudah.
- Token `"indra-pramudya-123"` tersimpan polos di binary (cek dengan `strings aplikasi.exe` atau String references di x64dbg). Hardcoded secret bukan rahasia.

## Tools yang Dipakai

- **x64dbg**: debugging, analisis, dan patch
- **g++** (MSYS2 UCRT64): compile
- **objdump / cmp**: verifikasi disassembly dan byte yang berubah
