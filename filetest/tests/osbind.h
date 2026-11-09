/* Host-only GEMDOS shim declarations. SPDX-License-Identifier: MIT */
long Fopen(const char *, int);
long Fcreate(const char *, int);
long Fclose(int);
long Fread(int, long, void *);
long Fwrite(int, long, const void *);
long Cconis(void);
long Cnecin(void);
