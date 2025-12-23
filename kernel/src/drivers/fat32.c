/* basic fat32 implementation */
/* sidenote: i thought this was harder lol */

#include "../utils/utils.h"

extern unsigned char disk_buffer[512];
unsigned char map_buffer[512];
unsigned char dir_buffer[512];
extern void read_ata(int lba, int buf);
extern void write_ata(int lba, char* data);
extern void output(char *str);
unsigned int bps = 0; // bytes per sector
unsigned int spc = 0; // sectors per cluster
unsigned int res_sect = 0; // reserved sectors
unsigned int num_of_fats = 0; // number of fats
unsigned int spf = 0; // sectors per fat
unsigned int root_clust = 0; // root cluster
unsigned int map_start = 0; // fat map starting point
unsigned int data_start = 0; // data starting point
unsigned int current_clust = 0; // current cluster
struct SearchResults {
    unsigned int cluster;
    int entry;
    int success;
    int attr;
};
char bps_str[200];
char filename_buf[50];

void fat_read_cluster(int cluster, int buf) {
  if (bps == 0) {
    return;
  }
  int lba = data_start + ((cluster - 2) * spc);
  read_ata(lba, buf);
}

void fat_write_cluster(int cluster, char *str) {
  unsigned char envelope[512];
  memset(envelope, 0, 512);
  for (int i = 0; i < 512; i++) {
    envelope[i] = str[i];
  }
  if (bps == 0) {
    return;
  }
  int lba = data_start + ((cluster - 2) * spc);
  write_ata(lba, envelope);
}

struct SearchResults fat_search(char *filename) {
  fat_read_cluster(current_clust, 3);
  for (int i = 0; i < 16; i++) {
    int entry_start = i * 32;
    if (dir_buffer[entry_start] == 0x00) {
      break;
    }
    if (dir_buffer[entry_start + 11] == 0x0F) {
        continue;
    }
    if ( (unsigned char)dir_buffer[entry_start] != 0xE5 ) {
      for (int o = 0; o < 11; o++) {
        filename_buf[o] = dir_buffer[entry_start+o];
      }
      filename_buf[11] = '\0';
      if (!strcmp(filename_buf, filename)) {
        int b26 = dir_buffer[entry_start + 26] & 0xFF;
        int b27 = dir_buffer[entry_start + 27] & 0xFF;
        int low = b26 | (b27 << 8);

        int b20 = dir_buffer[entry_start + 20] & 0xFF;
        int b21 = dir_buffer[entry_start + 21] & 0xFF;
        int high = b20 | (b21 << 8);

        unsigned int cluster = (high << 16) | low;
        struct SearchResults result = {
            cluster,
            entry_start,
            1,
            dir_buffer[entry_start + 11]
        };
        return result;
      }
    }
  }
  struct SearchResults fail = {
    0,
    0,
    0,
    0
  };
  return fail;
}

void fat_ls(int cluster) {  
  fat_read_cluster(cluster, 3);
  for (int i = 0; i < 16; i++) {
    int entry_start = i * 32;
    if (dir_buffer[entry_start] == 0x00) {
      break;
    }
    if (dir_buffer[entry_start + 11] == 0x0F) {
        continue;
    }
    if (dir_buffer[entry_start] == 0xE5) {
        continue;
    }
    if (dir_buffer[entry_start + 11] == 0x0F) {
        continue;
    }
    if (dir_buffer[entry_start + 11] == 0x10) {
      output("/");
    }
    for (int o = 0; o < 11; o++) {
      filename_buf[o] = dir_buffer[entry_start+o];
    }
    filename_buf[11] = '\0';
    output(filename_buf);
    output("  Cluster: ");
    char cluster_str[20];
    int_to_str(dir_buffer[entry_start + 26], cluster_str);
    output(cluster_str);
    output("\n");
    memset(cluster_str, 0, 20);
  }
}

int fat_find_free_cluster() {
    read_ata(map_start, 2);
    for (int i = 2; i < 128; i++) {
        int sp = i * 4;
        unsigned int a = map_buffer[sp] & 0xFF;
        unsigned int b = map_buffer[sp+1] & 0xFF;
        unsigned int c = map_buffer[sp+2] & 0xFF;
        unsigned int d = map_buffer[sp+3] & 0xFF;
        
        unsigned int entry = a | (b << 8) | (c << 16) | (d << 24);
        if (entry == 0) {
            map_buffer[sp] = 0xFF;
            map_buffer[sp + 1] = 0xFF;
            map_buffer[sp + 2] = 0xFF;
            map_buffer[sp + 3] = 0x0F;
            write_ata(map_start, map_buffer);
            return i;
        }
    }
    return 0;
}

void fat_overwrite_file(int cluster, char *data, int entry) {
    fat_write_cluster(cluster, data);
    fat_read_cluster(current_clust, 3);
    dir_buffer[entry + 28] = strlen(data);
    fat_write_cluster(current_clust, dir_buffer);
}

void fat_create_file(char *filename, char *data, char *extension) {
  int ext_included = 0;
  char blyat[12];
  for (int o = 0; o < 11; o++) {
    blyat[o] = 0x20;
  }
  for (int o = 0; o < 7; o++) {
    if (filename[o] == '\0') {
      break;
    }
    blyat[o] = filename[o];
  }
  blyat[7] = '.';
  blyat[8] = 't';
  blyat[9] = 'x';
  blyat[10] = 't';
  blyat[11] = '\0';
  struct SearchResults rslt = fat_search(blyat);
  if (rslt.success == 1) {
    fat_overwrite_file(rslt.cluster, data, rslt.entry);
    return;
  }
  blyat[7] = '.';
  blyat[8] = 'i';
  blyat[9] = 's';
  blyat[10] = 'l';
  rslt = fat_search(blyat);
  if (rslt.success == 1) {
    fat_overwrite_file(rslt.cluster, data, rslt.entry);
    return;
  }
  int free_cluster = fat_find_free_cluster();
  fat_read_cluster(current_clust, 3);
  for (int i = 0; i < 16; i++) {
    int entry_start = i * 32;
    if (dir_buffer[entry_start] == 0x00 || dir_buffer[entry_start] == 0xE5) {
        for (int o = 0; o < 11; o++) {
            dir_buffer[entry_start+o] = 0x20;
        }
        for (int o = 0; o < 7; o++) {
            if (filename[o] == '\0') {
                break;
            }
            dir_buffer[entry_start+o] = filename[o];
        }
        dir_buffer[entry_start+7] = '.';
        if (extension[0] == NULL) {
          dir_buffer[entry_start+8] = 't';
          dir_buffer[entry_start+9] = 'x';
          dir_buffer[entry_start+10] = 't';
        } else {
          dir_buffer[entry_start+8] = extension[0];
          dir_buffer[entry_start+9] = extension[1];
          dir_buffer[entry_start+10] = extension[2];
        }
        dir_buffer[entry_start + 11] = 0x20;
        dir_buffer[entry_start + 26] = free_cluster;
        dir_buffer[entry_start + 27] = 0;
        uint32_t size = strlen(data);
        dir_buffer[entry_start + 28] = (size & 0xFF);
        dir_buffer[entry_start + 29] = (size >> 8) & 0xFF;
        dir_buffer[entry_start + 30] = (size >> 16) & 0xFF;
        dir_buffer[entry_start + 31] = (size >> 24) & 0xFF;
        dir_buffer[entry_start + 30] = 0;
        dir_buffer[entry_start + 31] = 0;
        fat_write_cluster(current_clust, dir_buffer);
        fat_write_cluster(free_cluster, data);
        return;
    }
  }
  return;
}

void fat_create_dir(char *dirname) {
  int free_cluster = fat_find_free_cluster();
  fat_read_cluster(current_clust, 3);
  for (int i = 0; i < 16; i++) {
    int entry_start = i * 32;
    if (dir_buffer[entry_start] == 0x00 || dir_buffer[entry_start] == 0xE5) {
        for (int o = 0; o < 11; o++) {
            dir_buffer[entry_start+o] = 0x20;
        }
        for (int o = 0; o < 8; o++) {
            if (dirname[o] == '\0') {
                break;
            }
            dir_buffer[entry_start+o] = dirname[o];
        }
        dir_buffer[entry_start+8] = ' ';
        dir_buffer[entry_start+9] = ' ';
        dir_buffer[entry_start+10] = ' ';
        dir_buffer[entry_start + 11] = 0x10;
        dir_buffer[entry_start + 26] = free_cluster & 0xFF;
        dir_buffer[entry_start + 27] = (free_cluster >> 8) & 0xFF;
        dir_buffer[entry_start + 28] = 0;
        dir_buffer[entry_start + 30] = 0;
        dir_buffer[entry_start + 31] = 0;
        fat_write_cluster(current_clust, dir_buffer);
        memset(dir_buffer, 0, 512);
        dir_buffer[0] = '.';
        memset(&dir_buffer[1], 0x20, 10);
        dir_buffer[11] = 0x10;
        dir_buffer[26] = free_cluster & 0xFF;
        dir_buffer[27] = (free_cluster >> 8) & 0xFF;
        dir_buffer[32] = '.';
        dir_buffer[32 + 1] = '.';
        memset(&dir_buffer[34], 0x20, 9);
        dir_buffer[32 + 11] = 0x10;
        dir_buffer[32 + 26] = 0;
        dir_buffer[32 + 27] = 0;
        fat_write_cluster(free_cluster, dir_buffer);
        return;
    }
  }
  return;
}

void fat32_init() {
    read_ata(0, 1);
    bps = disk_buffer[11] | (disk_buffer[12] << 8);
    if (bps == 0) {
        output("Fat32 init failed.\n");
        return;
    }
    spc = disk_buffer[13];
    res_sect = disk_buffer[14] | (disk_buffer[15] << 8);
    num_of_fats = disk_buffer[16];
    unsigned int spf1 = disk_buffer[36] & 0xFF;
    unsigned int spf2 = disk_buffer[37] & 0xFF; 
    unsigned int spf3 = disk_buffer[38] & 0xFF; 
    unsigned int spf4 = disk_buffer[39] & 0xFF; 
    spf = spf1 | (spf2 << 8) | (spf3 << 16) | (spf4 << 24);
    int rcl1 = disk_buffer[44] & 0xFF;
    int rcl2 = disk_buffer[45] & 0xFF; 
    int rcl3 = disk_buffer[46] & 0xFF; 
    int rcl4 = disk_buffer[47] & 0xFF; 
    root_clust = rcl1 | (rcl2 << 8) | (rcl3 << 16) | (rcl4 << 24);
    map_start = res_sect;
    data_start = res_sect + (num_of_fats * spf);
    current_clust = root_clust;
    output("SPC: ");
    char spc_str[10];
    int_to_str(spc, spc_str);
    output(spc_str);
    output("\n");
    output("Fat32 initialized.\n");
}