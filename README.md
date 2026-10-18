# Belajar Reverse Engineering: Bypass Cek Token dengan Patch 1 Byte

Latihan dasar reverse engineering di Windows (x86-64). Kita membuat program kecil yang meminta token, lalu "meng-crack"-nya dengan **mengubah satu byte** di file `.exe` agar token apa pun diterima.

> ⚠️ Untuk tujuan edukasi saja. Praktikkan hanya pada program milik sendiri.

## Isi Repository

| File | Keterangan |
|------|------------|
| `aplikasi.cpp` | Source code program pengecek token |
| `aplikasi.exe` | Hasil compile asli (hanya menerima token yang benar) |
| `cracked.exe`  | Versi yang sudah di-patch (menerima token apa pun) |

## 1. Program Target

```cpp
if (token == "recoon-1337") {
    cout << "Selamat menikmati aplikasi kami!" << endl;
} else {
    cout << "Token ditolak!" << endl;
}
```

Compile (MSYS2 / MinGW-w64, g++):

```bash
g++ aplikasi.cpp -o aplikasi.exe
```

## 2. Analisis: Cari Titik Pengecekan

Disassemble dengan `objdump`:

```bash
objdump -d -M intel aplikasi.exe
```

Di fungsi `main` terlihat bagian ini:

```asm
140001523:  e8 f8 10 00 00   call  operator==(string, const char*)  ; bandingkan token
140001528:  84 c0            test  al, al                            ; hasil == 0 (salah)?
14000152a:  74 2a            je    140001556                         ; kalau salah -> lompat ke "Token ditolak!"
14000152c:  ...                                                      ; kalau benar -> "Selamat menikmati..."
```

Logikanya:
- `operator==` mengembalikan `1` (benar) atau `0` (salah) di register `al`.
- `test al, al` + `je` → **jika hasilnya 0, lompat** ke cabang "Token ditolak!".
- Jika tidak lompat, eksekusi lanjut ke cabang "Selamat menikmati...".

Jadi kunci seluruh pengecekan ada di **satu instruksi lompat bersyarat**.

## 3. Patch

Instruksi `74 2a` artinya `je +0x2a` (lompat 0x2A byte ke depan). Kalau offset-nya diubah jadi `00`, lompatannya mendarat tepat di instruksi berikutnya — artinya **lompat atau tidak, hasilnya sama**: masuk ke cabang sukses.

| | Byte | Instruksi |
|---|---|---|
| Sebelum | `74 2A` | `je 140001556` (ke "Token ditolak!") |
| Sesudah | `74 00` | `je 14000152c` (ke instruksi berikutnya = sukses) |

Lokasi di file: **offset `0x92B`** (byte `2A` → `00`).

Cara menghitung offset file dari alamat virtual (lihat `objdump -h aplikasi.exe`):

```
file_offset = VA - ImageBase - .text VMA + .text FileOffset
            = 0x14000152B - 0x140001000 + 0x400
            = 0x92B
```

Patch bisa dilakukan dengan hex editor (HxD, dll.), atau lewat terminal:

```bash
cp aplikasi.exe cracked.exe
printf '\x00' | dd of=cracked.exe bs=1 seek=$((0x92B)) conv=notrunc
```

Verifikasi hanya 1 byte yang berubah:

```bash
cmp -l aplikasi.exe cracked.exe
# 2348  52   0   (posisi 1-based, nilai oktal: 052 = 0x2A -> 0x00)
```

## 4. Hasil

```
> aplikasi.exe
Masukkan token: asal
Token ditolak!

> cracked.exe
Masukkan token: asal
Selamat menikmati aplikasi kami!
```

## Alternatif Patch Lain

Ada beberapa cara lain untuk mencapai hasil yang sama:
- Ganti `74 2a` dengan `90 90` (`nop nop`) — hapus lompatannya sama sekali.
- Ganti `74` (`je`) dengan `75` (`jne`) — logika terbalik: token salah diterima, token benar justru ditolak.

## Pelajaran

- Pengecekan `if` di source code berubah menjadi pasangan `cmp/test` + **conditional jump** di assembly. Itulah target utama saat cracking sederhana.
- Validasi yang sepenuhnya dilakukan di sisi klien bisa di-bypass dengan mudah.
- Token `"recoon-1337"` juga tersimpan polos di binary (cek dengan `strings aplikasi.exe`) — hardcoded secret bukan rahasia.

## Tools yang Dipakai

- **g++** (MSYS2 UCRT64) — compile
- **objdump** — disassembly & info section
- **xxd / cmp / dd** — lihat, bandingkan, dan patch byte
