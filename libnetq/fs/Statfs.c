/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/fs/Statfs.h"

const char* NQStatfsTypeStr(uint64_t type)
{
  switch (type) {
  case 0x0000ADF5: return "adfs";
  case 0x0000ADFF: return "affs";
  case 0x00000187: return "autofs";
  case 0x42465331: return "befs";
  case 0x1BADFACE: return "bfs";
  case 0x42494E4D: return "binfmt_misc";
  case 0xCAFE4A11: return "bpf_fs";
  case 0x9123683E: return "btrfs";
  case 0x0027E0EB: return "cgroupfs";
  case 0x63677270: return "cgroup2fs";
  case 0xFF534D42: return "cifs";
  case 0x73757245: return "coda";
  case 0x012FF7B7: return "coh";
  case 0x62656570: return "configfs";
  case 0x28CD3D45: return "cramfs";
  case 0x453DCD28: return "cramfs-wend";
  case 0x64626720: return "debugfs";
  case 0x00001373: return "devfs";
  case 0x00001CD1: return "devpts";
  case 0x0000F15F: return "ecryptfs";
  case 0xDE5E81E4: return "efivarfs";
  case 0x00414A53: return "efs";
  case 0xE0F5E1E2: return "erofs";
  case 0x2011BAB0: return "exfat";
  case 0x0000EF51: return "ext2";
  case 0x0000EF53: return "ext2/ext3";
  case 0xF2F52010: return "f2fs";
  case 0x65735546: return "fuseblk";
  case 0x65735543: return "fusectl";
  case 0x01161970: return "gfs/gfs2";
  case 0x00004244: return "hfs";
  case 0x0000482B: return "hfs+";
  case 0x00004858: return "hfsx";
  case 0x958458F6: return "hugetlbfs";
  case 0x00009660:
  case 0x00004004:
  case 0x00004000: return "isofs";
  case 0x000072B6: return "jffs2";
  case 0x3153464A: return "jfs";
  case 0x0000137F: return "minix";
  case 0x0000138F: return "minix (30 char.)";
  case 0x00002468: return "minix v2";
  case 0x00002478: return "minix v2 (30 char.)";
  case 0x00004D5A: return "minix3";
  case 0x19800202: return "mqueue";
  case 0x00004D44: return "msdos";
  case 0x0000564C: return "novell";
  case 0x00006969: return "nfs";
  case 0x6E667364: return "nfsd";
  case 0x00003434: return "nilfs";
  case 0x6E736673: return "nsfs";
  case 0x5346544E:
  case 0x7366746E: return "ntfs";
  case 0x00009FA1: return "openprom";
  case 0x7461636F: return "ocfs2";
  case 0x794C7630: return "overlayfs";
  case 0xAAD7AAEA: return "panfs";
  case 0x50495045: return "pipefs";
  case 0x6165676C: return "pstorefs";
  case 0x00009FA0: return "proc";
  case 0x0000002F: return "qnx4";
  case 0x68191122: return "qnx6";
  case 0x858458F6: return "ramfs";
  case 0x52654973: return "reiserfs";
  case 0x00007275: return "romfs";
  case 0x67596969: return "rpc_pipefs";
  case 0x73636673: return "securityfs";
  case 0xF97CFF8C: return "selinux";
  case 0x43415D53: return "smackfs";
  case 0x0000517B: return "smb";
  case 0xFE534D42: return "smb2";
  case 0x534F434B: return "sockfs";
  case 0x73717368: return "squashfs";
  case 0x62656572: return "sysfs";
  case 0x012FF7B6: return "sysv2";
  case 0x012FF7B5: return "sysv4";
  case 0x01021994: return "tmpfs";
  case 0x74726163: return "tracefs";
  case 0x24051905: return "ubifs";
  case 0x15013346: return "udf";
  case 0x00011954:
  case 0x54190100: return "ufs";
  case 0x00009FA2: return "usbdevfs";
  case 0x01021997: return "v9fs";
  case 0xA501FCF5: return "vxfs";
  case 0x565A4653: return "vzfs";
  case 0xABBA1974: return "xenfs";
  case 0x012FF7B4: return "xenix";
  case 0x58465342: return "xfs";
  case 0x012FD16D: return "xia";
  case 0x2FC12FC1: return "zfs";
  case 0x58295829: return "zsmallocfs";
  case 0x09041934: return "anon-inode FS";
  default: return NULL;
  }
}
