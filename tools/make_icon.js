const fs = require("fs");
const zlib = require("zlib");
const path = require("path");

function crc32(buf) {
  let table = crc32.table;
  if (!table) {
    table = crc32.table = new Uint32Array(256);
    for (let i = 0; i < 256; i++) {
      let c = i;
      for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
      table[i] = c >>> 0;
    }
  }
  let c = 0xffffffff;
  for (const b of buf) c = table[(c ^ b) & 0xff] ^ (c >>> 8);
  return (c ^ 0xffffffff) >>> 0;
}

function chunk(type, data) {
  const t = Buffer.from(type, "ascii");
  const len = Buffer.alloc(4);
  len.writeUInt32BE(data.length, 0);
  const crc = Buffer.alloc(4);
  crc.writeUInt32BE(crc32(Buffer.concat([t, data])), 0);
  return Buffer.concat([len, t, data, crc]);
}

function png(width, height, draw) {
  const raw = Buffer.alloc((width * 4 + 1) * height);
  for (let y = 0; y < height; y++) {
    raw[y * (width * 4 + 1)] = 0;
    for (let x = 0; x < width; x++) {
      const [r, g, b, a] = draw(x, y, width, height);
      const i = y * (width * 4 + 1) + 1 + x * 4;
      raw[i] = r;
      raw[i + 1] = g;
      raw[i + 2] = b;
      raw[i + 3] = a;
    }
  }

  const ihdr = Buffer.alloc(13);
  ihdr.writeUInt32BE(width, 0);
  ihdr.writeUInt32BE(height, 4);
  ihdr[8] = 8;
  ihdr[9] = 6;
  ihdr[10] = 0;
  ihdr[11] = 0;
  ihdr[12] = 0;

  return Buffer.concat([
    Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]),
    chunk("IHDR", ihdr),
    chunk("IDAT", zlib.deflateSync(raw)),
    chunk("IEND", Buffer.alloc(0)),
  ]);
}

function drawIcon(x, y, w, h) {
  const sx = x / w;
  const sy = y / h;

  // Rounded-square-ish alpha mask.
  const pad = 0.06;
  const r = 0.16;
  const dx = Math.max(pad - sx, 0, sx - (1 - pad));
  const dy = Math.max(pad - sy, 0, sy - (1 - pad));
  const outsideCorner = dx > 0 && dy > 0 && Math.sqrt(dx * dx + dy * dy) > r;
  if (outsideCorner || sx < pad * 0.4 || sx > 1 - pad * 0.4 || sy < pad * 0.4 || sy > 1 - pad * 0.4) {
    return [0, 0, 0, 0];
  }

  let r0 = 24, g0 = 24, b0 = 28, a0 = 255;

  // Subtle diagonal panel shine.
  const shine = Math.floor((sx + sy) * 18);
  r0 += shine;
  g0 += shine;
  b0 += shine;

  // Accent border.
  const border = sx < 0.11 || sx > 0.89 || sy < 0.11 || sy > 0.89;
  if (border) return [122, 92, 255, 255];

  // Blocky "J" in green.
  const j =
    (sx > 0.22 && sx < 0.50 && sy > 0.22 && sy < 0.32) ||
    (sx > 0.36 && sx < 0.48 && sy > 0.22 && sy < 0.66) ||
    (sx > 0.22 && sx < 0.48 && sy > 0.56 && sy < 0.68) ||
    (sx > 0.20 && sx < 0.32 && sy > 0.48 && sy < 0.62);

  if (j) return [0, 255, 153, 255];

  // Blocky "S" in light gray.
  const s =
    (sx > 0.54 && sx < 0.78 && sy > 0.22 && sy < 0.32) ||
    (sx > 0.54 && sx < 0.66 && sy > 0.22 && sy < 0.45) ||
    (sx > 0.54 && sx < 0.78 && sy > 0.40 && sy < 0.50) ||
    (sx > 0.66 && sx < 0.78 && sy > 0.45 && sy < 0.68) ||
    (sx > 0.54 && sx < 0.78 && sy > 0.58 && sy < 0.68);

  if (s) return [230, 230, 230, 255];

  return [r0, g0, b0, a0];
}

function makeIco(sizes) {
  const images = sizes.map((s) => png(s, s, drawIcon));
  const header = Buffer.alloc(6);
  header.writeUInt16LE(0, 0);
  header.writeUInt16LE(1, 2);
  header.writeUInt16LE(images.length, 4);

  const entries = [];
  let offset = 6 + images.length * 16;

  for (let i = 0; i < images.length; i++) {
    const s = sizes[i];
    const img = images[i];
    const e = Buffer.alloc(16);
    e[0] = s >= 256 ? 0 : s;
    e[1] = s >= 256 ? 0 : s;
    e[2] = 0;
    e[3] = 0;
    e.writeUInt16LE(1, 4);
    e.writeUInt16LE(32, 6);
    e.writeUInt32LE(img.length, 8);
    e.writeUInt32LE(offset, 12);
    entries.push(e);
    offset += img.length;
  }

  return Buffer.concat([header, ...entries, ...images]);
}

fs.mkdirSync("resources", { recursive: true });
fs.writeFileSync(path.join("resources", "jesterstep.ico"), makeIco([16, 32, 48, 64, 128, 256]));
console.log("Created resources\\jesterstep.ico");