/*
mdio-tool allow for direct access to mdio registers in a network phy.

Routines are based on mii-tool: http://freecode.com/projects/mii-tool

mdio-tool comes with ABSOLUTELY NO WARRANTY; Use with care!

Copyright (C) 2013 Pieter Voorthuijsen

mdio-tool is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

mdio-tool is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with mdio-tool.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>

#ifndef __GLIBC__
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#endif
#include "mii.h"

#define MAX_ETH		8		/* Maximum # of interfaces */

#define REGCR 0x0D
#define ADDAR 0x0E

#define REGCR_MMD_A 0x001F
#define REGCR_MMD_D 0x401F

#define MDIO_TOP_ADDR 0x1F
#define MMD_TOP_ADDR 0x4D1

static int skfd = -1;		/* AF_INET socket for ioctl() calls. */
static struct ifreq ifr;

/*--------------------------------------------------------------------*/

static int mdio_read(int skfd, uint16_t location)
{
	struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;
	mii->reg_num = location;
	if (ioctl(skfd, SIOCGMIIREG, &ifr) < 0) {
	fprintf(stderr, "SIOCGMIIREG on %s failed: %s\n", ifr.ifr_name,
		strerror(errno));
	return -1;
	}
	return mii->val_out;
}

static void mdio_write(int skfd, uint16_t location, uint16_t value)
{
	struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;
	mii->reg_num = location;
	mii->val_in = value;
	if (ioctl(skfd, SIOCSMIIREG, &ifr) < 0) {
	fprintf(stderr, "SIOCSMIIREG on %s failed: %s\n", ifr.ifr_name,
		strerror(errno));
	}
}

static int mmd_read(int skfd, uint16_t location)
{
	mdio_write(skfd, REGCR, REGCR_MMD_A);
	mdio_write(skfd, ADDAR, location);
	mdio_write(skfd, REGCR, REGCR_MMD_D);

	return mdio_read(skfd, ADDAR);
}

static void mmd_write(int skfd, uint16_t location, uint16_t value)
{
	mdio_write(skfd, REGCR, REGCR_MMD_A);
	mdio_write(skfd, ADDAR, location);
	mdio_write(skfd, REGCR, REGCR_MMD_D);
	mdio_write(skfd, ADDAR, value);
}

int main(int argc, char **argv)
{
	uint16_t addr, val;

	if(argc < 2) {
		printf("Usage mdio-tool [r/w] [dev] [reg] [val]\n");
		return 0;
	}

	/* Open a basic socket. */
	if ((skfd = socket(AF_INET, SOCK_DGRAM,0)) < 0) {
		perror("socket");
		return -1;
	}

	/* Get the vitals from the interface. */
	strncpy(ifr.ifr_name, argv[2], IFNAMSIZ);
	if (ioctl(skfd, SIOCGMIIPHY, &ifr) < 0) {
		if (errno != ENODEV)
			fprintf(stderr, "SIOCGMIIPHY on '%s' failed: %s\n",
				argv[2], strerror(errno));
		return -1;
	}

	if (argv[1][0] == 'r') {
	addr = strtol(argv[3], NULL, 16);

		if (addr <= MDIO_TOP_ADDR) {
			val = mdio_read(skfd, addr);
		}
		else if (addr <= MMD_TOP_ADDR) {
			val = mmd_read(skfd, addr);
		}
		else {
			printf ("Not supported MMD range\n");
			return -1;
		}
		printf("read addr 0x%.4x = 0x%.4x\n", addr, val);
	}
	else if (argv[1][0] == 'w') {
		addr = strtol(argv[3], NULL, 16);
		val = strtol(argv[4], NULL, 16);
		printf("write val %#.4x to addr %#.4x\n", val, addr);

		if (addr <= MDIO_TOP_ADDR) {
			mdio_write(skfd, addr, val);
		}
		else if (addr <= MMD_TOP_ADDR) {
			mmd_write(skfd, addr, val);
		}
		else {
			printf ("Not supported MMD range\n");
			return -1;
		}
	}
	else {
		printf("Fout!\n");
	}

	close(skfd);
}
