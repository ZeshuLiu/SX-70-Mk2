#!/usr/bin/env python3
"""Convert a monochrome BMP image into a C header."""

from __future__ import annotations

import argparse
import datetime
import pathlib
import re
import struct
from typing import Iterable, List


def _calc_luminance(rgb: Iterable[int]) -> float:
	r, g, b = rgb
	return 0.299 * r + 0.587 * g + 0.114 * b


def _sanitize_identifier(name: str) -> str:
	ident = re.sub(r"\W", "_", name)
	if not ident:
		ident = "image"
	if ident[0].isdigit():
		ident = f"_{ident}"
	return ident


def _pack_bits(bits: List[int]) -> bytes:
	packed = bytearray()
	for i in range(0, len(bits), 8):
		byte = 0
		chunk = bits[i : i + 8]
		chunk += [0] * (8 - len(chunk))
		for offset, value in enumerate(chunk):
			byte |= (value & 0x01) << (7 - offset)
		packed.append(byte)
	return bytes(packed)


def _read_image(path: pathlib.Path, prefer: str, invert: bool) -> tuple[int, int, bytes]:
	with path.open("rb") as fh:
		file_header = fh.read(14)
		if len(file_header) != 14:
			raise ValueError("Incomplete BMP file header")
		signature, _, _, _, pixel_offset = struct.unpack("<2sIHHI", file_header)
		if signature != b"BM":
			raise ValueError("Input file is not a BMP image")

		dib_size_data = fh.read(4)
		if len(dib_size_data) != 4:
			raise ValueError("Incomplete DIB header size")
		dib_size = struct.unpack("<I", dib_size_data)[0]
		dib_data = fh.read(dib_size - 4)
		if len(dib_data) != dib_size - 4:
			raise ValueError("Incomplete DIB header data")

		if dib_size < 40:
			raise ValueError("Unsupported BMP DIB header size")

		width, height, planes, bpp, compression, raw_size, _, _, colors_used, _ = struct.unpack(
			"<iiHHIIiiII", dib_data[:36]
		)

		if planes != 1 or bpp != 1:
			raise ValueError("Only monochrome (1-bit) BMP files are supported")
		if compression != 0:
			raise ValueError("Compressed BMP images are not supported")

		palette_entries = colors_used if colors_used else 1 << bpp
		palette: List[tuple[int, int, int]] = []
		for _ in range(palette_entries):
			entry = fh.read(4)
			if len(entry) != 4:
				raise ValueError("Incomplete BMP colour palette")
			b, g, r, _ = struct.unpack("<BBBB", entry)
			palette.append((r, g, b))

		current = fh.tell()
		if pixel_offset < current:
			raise ValueError("Unexpected pixel data offset")
		fh.seek(pixel_offset)

		height_abs = abs(height)
		stride = ((width * bpp + 31) // 32) * 4
		if raw_size == 0:
			raw_size = stride * height_abs
		pixel_data = fh.read(raw_size)
		expected = stride * height_abs
		if len(pixel_data) < expected:
			raise ValueError("Incomplete BMP pixel data")

	luminance = [_calc_luminance(rgb) for rgb in palette]
	dark_index = luminance.index(min(luminance))
	light_index = luminance.index(max(luminance))
	on_index = dark_index if prefer == "dark" else light_index
	if invert:
		alternatives = [idx for idx in range(len(palette)) if idx != on_index]
		if not alternatives:
			raise ValueError("Cannot invert palette mapping")
		on_index = alternatives[0]

	rows: List[bytes] = []
	for row_idx in range(height_abs):
		start = row_idx * stride
		rows.append(pixel_data[start : start + stride])

	if height > 0:
		rows.reverse()

	bytes_per_row = (width + 7) // 8
	packed = bytearray()
	for row in rows:
		row_indices: List[int] = []
		for byte in row:
			for offset in range(7, -1, -1):
				row_indices.append((byte >> offset) & 0x1)
		row_indices = row_indices[:width]

		mapped = [1 if index == on_index else 0 for index in row_indices]
		mapped.extend([0] * ((8 - (len(mapped) % 8)) % 8))
		packed.extend(_pack_bits(mapped)[:bytes_per_row])

	return width, height_abs, bytes(packed)


def _render_header(
	image_path: pathlib.Path,
	width: int,
	height: int,
	data: bytes,
	struct_name: str,
	data_name: str,
) -> str:
	guard = f"{struct_name.upper()}_H"
	byte_literals = [f"0x{value:02X}" for value in data]
	lines = []
	for i in range(0, len(byte_literals), 12):
		lines.append(", ".join(byte_literals[i : i + 12]))
	data_block = ",\n    ".join(lines)

	return (
		f"#ifndef {guard}\n"
		f"#define {guard}\n\n"
		f"#include \"pico/stdlib.h\"\n"
		f"#include \"lib/image.h\"\n\n"
		f"static const uint8_t {data_name}[{len(data)}] = {{\n"
		f"    {data_block}\n"
		f"}};\n\n"
		f"static const ssd1306_image_t {struct_name} = {{\n"
		f"    .width = {width},\n"
		f"    .height = {height},\n"
		f"    .length = sizeof({data_name}),\n"
		f"    .data = {data_name},\n"
		f"}};\n\n"
		f"#endif // {guard}\n"
	)


def main() -> None:
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("image", type=pathlib.Path, help="Path to a 1-bit BMP image")
	parser.add_argument(
		"--output",
		type=pathlib.Path,
		help="Output header path (defaults to input stem with .h)",
	)
	parser.add_argument(
		"--name",
		help="Override the C identifier base name (defaults to input stem)",
	)
	parser.add_argument(
		"--prefer",
		choices=("dark", "light"),
		default="dark",
		help="Which palette entry should be treated as an active pixel",
	)
	parser.add_argument(
		"--invert",
		action="store_true",
		help="Invert the mapping between palette and active pixels",
	)

	args = parser.parse_args()

	input_path = args.image.resolve()
	if not input_path.exists():
		raise SystemExit(f"Input file '{input_path}' does not exist")

	width, height, data = _read_image(input_path, args.prefer, args.invert)
	stem = args.name or input_path.stem
	base_ident = _sanitize_identifier(stem)
	struct_name = base_ident
	data_name = f"{base_ident}_data"

	output_path = args.output or input_path.with_suffix(".h")
	header_content = _render_header(input_path, width, height, data, struct_name, data_name)
	output_path.write_text(header_content)


if __name__ == "__main__":
	main()
