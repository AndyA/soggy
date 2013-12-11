#!/bin/bash 

src="media/ChID-BLITS-EBU-Narration.aac"
dst="hls"

mkdir -p "$dst"

set -x

ffmpeg -y \
  -i "$src" \
  -c:a copy \
  -map 0:0 \
  -f segment \
  -segment_time 4 \
  -segment_format mpegts \
  -segment_list "$dst.m3u8" \
  -segment_list_size 1800 \
  -segment_list_flags live \
  -segment_list_type m3u8 \
  "$dst/%08d.ts" < /dev/null 

# vim:ts=2:sw=2:sts=2:et:ft=sh
