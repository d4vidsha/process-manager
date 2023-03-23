/*
	process.c, a dummy process used in COMP30023 2023 Project 1
	Authored by Steven Tang
	Reference: signalfd, waitpid, getopt_long
	https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
	Comments deliberately kept sparse and brief
	Please report any issues on Ed
	Last updated 21/03/23
*/

#define _POSIX_C_SOURCE 1

#include <err.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <unistd.h>

static long pid = 0;
static int verbose_flag = 0;
typedef enum { STOP = 1, CONTINUE = 2, TERM = 3, START = 0 } Op;

void read_store_dword(Op op, uint8_t hash_content[128], size_t* dest_index);
void store_process_name(const char* process_name, uint8_t hash_content[128],
						size_t* dest_index);
void sha256_hash(char hash_hexstring[65], const uint8_t* buf,
				 const uint64_t nbyte);

int main(int argc, char* argv[]) {
	int c;
	char* process_name;
	static struct option long_options[] = {
		{"verbose", no_argument, &verbose_flag, 1},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}};
	int option_index;

	int sfd;
	ssize_t s;
	sigset_t mask;
	struct signalfd_siginfo fdsi;

	size_t dest_index;
	uint8_t sha_content[128];
	char hash[65];

	while (1) {
		option_index = 0;
		c = getopt_long(argc, argv, "hv", long_options, &option_index);
		if (c == -1) {
			break;
		}

		switch (c) {
		case 0: break;
		case 'v': verbose_flag = 1; break;
		case 'h':
			printf("Usage: %s [-v|--verbose] <process-name>\n", argv[0]);
			exit(EXIT_SUCCESS);
		}
	}
	pid = getpid();
	if (verbose_flag) {
		fprintf(stderr, "[process.c (%ld)] ppid: %ld\n", pid, (long)getppid());
	}
	if (optind + 1 != argc) {
		fprintf(stderr,
				"[process.c] Error: Less or more arguments than expected\n");
		exit(EXIT_FAILURE);
	}
	process_name = argv[optind++];

	memset(sha_content, 0, 128);
	dest_index = 0;
	store_process_name(process_name, sha_content, &dest_index);

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGTSTP);
	sigaddset(&mask, SIGCONT);
	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
		err(EXIT_FAILURE, "sigprocmask");
	}
	sfd = signalfd(-1, &mask, 0);
	if (sfd == -1) {
		err(EXIT_FAILURE, "signalfd");
	}

	/* Synchronisation at the start */
	/* Must be placed after signal setup to prevent race */
	read_store_dword(START, sha_content, &dest_index);

	for (;;) {
		s = read(sfd, &fdsi, sizeof(fdsi));
		if (s != sizeof(fdsi)) {
			err(EXIT_FAILURE, "read");
		}

		if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGTERM) {
			sigemptyset(&mask);
			sigaddset(&mask, SIGINT);
			sigaddset(&mask, SIGTERM);
			if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1) {
				err(EXIT_FAILURE, "sigprocmask");
			}

			if (verbose_flag) {
				fprintf(stderr,
						"[process.c (%ld)] handling SIGINT or SIGTERM\n", pid);
			}
			read_store_dword(TERM, sha_content, &dest_index);

			sha256_hash(hash, sha_content, 128 - 9);
			printf("%s\n", hash);

			exit(EXIT_SUCCESS);
		} else if (fdsi.ssi_signo == SIGTSTP) {
			sigemptyset(&mask);
			sigaddset(&mask, SIGTSTP);
			if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1) {
				err(EXIT_FAILURE, "sigprocmask");
			}

			if (verbose_flag) {
				fprintf(stderr, "[process.c (%ld)] handling SIGTSTP\n", pid);
			}
			read_store_dword(STOP, sha_content, &dest_index);
			raise(SIGSTOP);
		} else if (fdsi.ssi_signo == SIGCONT) {
			if (verbose_flag) {
				fprintf(stderr, "[process.c (%ld)] handling SIGCONT\n", pid);
			}
			/* Must be placed before read to prevent SIGCONT race */
			sigemptyset(&mask);
			sigaddset(&mask, SIGTSTP);
			if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
				err(EXIT_FAILURE, "sigprocmask");
			}
			read_store_dword(CONTINUE, sha_content, &dest_index);
		} else {
			fprintf(stderr, "Read unexpected signal\n");
		}
	}

	return 0;
}

void store(uint8_t* buf, size_t len, uint8_t* dest, size_t* dest_index) {
	size_t i;
	for (i = 0; i < len; i++) {
		*dest_index = (*dest_index + i) % (128 - 9);
		dest[*dest_index] ^= buf[i];
	}
}

void store_process_name(const char* process_name, uint8_t hash_content[128],
						size_t* dest_index) {
	if (verbose_flag) {
		fprintf(stderr, "[process.c (%ld)] started with process name %s\n",
				pid, process_name);
	}
	store((uint8_t*)process_name, strlen(process_name), hash_content,
		  dest_index);
}

void read_store_dword(Op op, uint8_t hash_content[128], size_t* dest_index) {
	uint8_t buf[5];
	size_t len, n;

	buf[0] = op;
	len = 1;
	if (verbose_flag) {
		fprintf(
			stderr,
			"[process.c (%ld)] reading 4 (Big Endian) bytes from stdin...\n",
			pid);
		fflush(stderr);
	}
	while (len < 5) {
		n = read(STDIN_FILENO, buf + len, 5 - len);
		if (n < 0) {
			err(EXIT_FAILURE, "read");
		}
		len += n;
	}

	if (verbose_flag) {
		fprintf(stderr,
				"[process.c (%ld)] op %d, time %d, hex bytes [%02x, %02x, "
				"%02x, %02x]\n",
				pid, op,
				((uint32_t)buf[1]) << 24 | ((uint32_t)buf[2]) << 16 |
					((uint32_t)buf[3]) << 8 | (uint32_t)buf[4],
				buf[1], buf[2], buf[3], buf[4]);
	}
	if (ioctl(STDIN_FILENO, FIONREAD, &n) == 0 && n > 0) {
		fprintf(stderr, "[process.c (%ld)] Error: Leftover bytes in stdin\n",
				pid);
		exit(EXIT_FAILURE);
	}

	/* Need to prevent race condition between SIGCONT and SIGTERM */
	if (op == CONTINUE || op == START) {
		while (1) {
			n = write(STDOUT_FILENO, &buf[4], 1);
			if (n < 0) {
				err(EXIT_FAILURE, "write");
			}
			if (n == 1) {
				break;
			}
		}
		fsync(STDOUT_FILENO);
		fflush(stdout);
	}
	if (verbose_flag) {
		fprintf(stderr, "[process.c (%ld)] wrote hex byte [%02x] to stdout\n",
				pid, buf[4]);
		fflush(stderr);
	}
	store(buf, 5, hash_content, dest_index);
}

/*****************************************************************************/
/* SHA-256 Hashing, implemented by Steven Tang */
/* Reference: RFC 6234 */

/* SHA-256 Functions: RFC 6234, FIPS 180-3 section 4.1.2 */
#define SHA_Ch(x, y, z) (((x) & (y)) ^ ((~(x)) & (z)))
#define SHA_Maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define SHA256_SHR(bits, word) ((word) >> (bits))
#define SHA256_ROTL(bits, word)                                               \
	(((word) << (bits)) | ((word) >> (32 - (bits))))
#define SHA256_ROTR(bits, word)                                               \
	(((word) >> (bits)) | ((word) << (32 - (bits))))

#define SHA256_BSIG0(word)                                                    \
	(SHA256_ROTR(2, word) ^ SHA256_ROTR(13, word) ^ SHA256_ROTR(22, word))
#define SHA256_BSIG1(word)                                                    \
	(SHA256_ROTR(6, word) ^ SHA256_ROTR(11, word) ^ SHA256_ROTR(25, word))
#define SHA256_SSIG0(word)                                                    \
	(SHA256_ROTR(7, word) ^ SHA256_ROTR(18, word) ^ SHA256_SHR(3, word))
#define SHA256_SSIG1(word)                                                    \
	(SHA256_ROTR(17, word) ^ SHA256_ROTR(19, word) ^ SHA256_SHR(10, word))

/* SHA-256 Initial Hash Values: FIPS 180-3 section 5.3.3 */
static uint32_t SHA256_H0[8] = {0x6A09E667, 0xBB67AE85, 0x3C6EF372,
								0xA54FF53A, 0x510E527F, 0x9B05688C,
								0x1F83D9AB, 0x5BE0CD19};

/* SHA-256 Constants: FIPS 180-3, section 4.2.2 */
static const uint32_t K[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
	0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
	0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
	0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
	0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
	0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

void sha256_init(uint32_t hash[8]);
void sha256_process(uint32_t message_block[16], uint32_t hash[8]);
void sha256_process_final(uint64_t nbyte, short leftover_bytes,
						  uint32_t last_block[16], uint32_t hash[8]);
void uint32_array_to_hex_string(char* out, uint32_t* in, unsigned long length);
#ifdef DEBUG
void print_uint32_array(uint32_t* arr, unsigned long length);
#endif

void sha256_hash(char hash_hexstring[65], const uint8_t* buf,
				 const uint64_t nbyte) {
	uint64_t i, num_blocks;
	uint32_t hash[8], last_block[16];
	short leftover_bytes;

	/* each block is 512 bits, or 64 bytes */
	/* last block needs to be processed differently for padding */
	leftover_bytes = nbyte % 64;
	num_blocks = leftover_bytes == 0 ? nbyte / 64 : nbyte / 64 + 1;

#ifdef DEBUG
	fprintf(stderr, "Blocks: %ld, Leftover bytes %d\n", num_blocks,
			leftover_bytes);
#endif

	/* init */
	sha256_init(hash);

	/* process */
	/* 64 = 512 bits per block / 8 bits for uint8_t */
	for (i = 0; i < (leftover_bytes == 0 ? num_blocks : num_blocks - 1); i++) {
		sha256_process((uint32_t*)(buf + i * 64), hash);
	}

	/* final block */
	memset(last_block, 0, 64);
	if (leftover_bytes != 0) {
		memcpy(last_block, buf + i * 64, leftover_bytes);
	}
	sha256_process_final(nbyte, leftover_bytes, last_block, hash);

	/* print to buffer as hex string */
	uint32_array_to_hex_string(hash_hexstring, hash, 8);
}

/* SHA-256 Initialization */
/* https://www.rfc-editor.org/rfc/rfc6234#section-6.1 */
void sha256_init(uint32_t hash[8]) {
	int i;
	for (i = 0; i < 8; i++) {
		hash[i] = SHA256_H0[i];
	}
}

/* SHA-256 Processing */
/* For each 32 * 16 = 512 block of bytes */
/* https://www.rfc-editor.org/rfc/rfc6234#section-6.2 */
void sha256_process(uint32_t message_block[16], uint32_t hash[8]) {
	int t;
	uint32_t a, b, c, d, e, f, g, h, t1, t2;
	uint32_t w[64];
	uint8_t* message_ptr;

	/* prepare message schedule */
	for (t = 0; t < 64; t++) {
		if (t < 16) {
			/* w[t] = ntohl(message_block[t]); */
			message_ptr = (uint8_t*)&message_block[t];
			w[t] = message_ptr[0] << 24 | message_ptr[1] << 16 |
				   message_ptr[2] << 8 | message_ptr[3];
		} else {
			w[t] = SHA256_SSIG1(w[t - 2]) + w[t - 7] +
				   SHA256_SSIG0(w[t - 15]) + w[t - 16];
		}
	}

	/* initialize working variables */
	a = hash[0];
	b = hash[1];
	c = hash[2];
	d = hash[3];
	e = hash[4];
	f = hash[5];
	g = hash[6];
	h = hash[7];

	/* main hash computation */
	for (t = 0; t < 64; t++) {
		t1 = h + SHA256_BSIG1(e) + SHA_Ch(e, f, g) + K[t] + w[t];
		t2 = SHA256_BSIG0(a) + SHA_Maj(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	/* calculate intermediate hash value H(i) */
	hash[0] += a;
	hash[1] += b;
	hash[2] += c;
	hash[3] += d;
	hash[4] += e;
	hash[5] += f;
	hash[6] += g;
	hash[7] += h;
}

/* SHA-256 processing for final block */
/* Padding - "1" followed by m "0"s followed by 64-bit integer */
/* https://www.rfc-editor.org/rfc/rfc6234#section-4.1 */
void sha256_process_final(uint64_t nbyte, short leftover_bytes,
						  uint32_t last_block[16], uint32_t hash[8]) {
	/* Append "1" bit */
	((uint8_t*)last_block)[leftover_bytes] = 1 << 7;

	/* Length will be in next block since it doesn't fit */
	if (leftover_bytes > 64 - 8 - 1) {
		sha256_process(last_block, hash);
		memset(last_block, 0, 64);
	}

	/* Set length, process last block */
	/*
		((uint64_t*)last_block)[7] = htobe64(nbyte * 8);
		last_block[14] = htonl(nbyte * 8 >> 32);
		last_block[15] = htonl(nbyte * 8 & 0xFFFFFFF0);
	*/
	((uint8_t*)last_block)[56] = (nbyte * 8 >> (64 - 8)) & 0xFF;
	((uint8_t*)last_block)[57] = (nbyte * 8 >> (64 - 16)) & 0xFF;
	((uint8_t*)last_block)[58] = (nbyte * 8 >> (64 - 24)) & 0xFF;
	((uint8_t*)last_block)[59] = (nbyte * 8 >> (64 - 32)) & 0xFF;
	((uint8_t*)last_block)[60] = (nbyte * 8 >> (64 - 40)) & 0xFF;
	((uint8_t*)last_block)[61] = (nbyte * 8 >> (64 - 48)) & 0xFF;
	((uint8_t*)last_block)[62] = (nbyte * 8 >> (64 - 56)) & 0xFF;
	((uint8_t*)last_block)[63] = (nbyte * 8 >> (64 - 64)) & 0xFF;

	sha256_process(last_block, hash);
}

#ifdef DEBUG
/* Prints a uint32 array */
void print_uint32_array(uint32_t* arr, unsigned long length) {
	unsigned long i;
	for (i = 0; i < length; i++) {
		fprintf(stderr, "%08x", arr[i]);
	}
	sprintf(stderr, "\n");
}
#endif

/* Returns a uint32 array in hex notation */
void uint32_array_to_hex_string(char* out, uint32_t* in,
								unsigned long length) {
	int i;
	for (i = 0; i < length; i++) {
		sprintf(out + (i * 8), "%08x", in[i]);
	}
	out[64] = 0;
}
