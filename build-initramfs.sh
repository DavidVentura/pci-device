#!/bin/bash
set -euo pipefail
(cd driver/ && make && cp gpu.ko ../initramfs/)
cd initramfs && find . | cpio --quiet -H newc -o | gzip -9 -n > ../initramfs.gz
