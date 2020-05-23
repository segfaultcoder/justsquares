#ifndef __WORLD
#define __WORLD
#include "libs/PerlinNoise.h"
#include "libs/json.hpp"
#include "block.hpp"
#include "player.hpp"
#include <fstream>
#include <iostream>
#include <sys/stat.h>

#define imin(a, b) (a<b?a:b)
#define imax(a, b) (a>b?a:b)

#define tiabs(a) (a<0?-a:a)

long getfilesize(string filename) {
	struct stat stat_buf;
	int rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}

class interact_data {
	public:

	bool top;
	bool bottom;
	bool left;
	bool right;

	interact_data() {}

	interact_data(bool _top, bool _bottom, bool _left, bool _right) {
		top = _top; bottom = _bottom; left = _left; right = _right;
	}
};

class json_interact_data {
	public:

	string name;
	bool top;
	bool bottom;
	bool left;
	bool right;

	json_interact_data() {}

	json_interact_data(string _name, bool _top, bool _bottom, bool _left, bool _right) {
		name = _name; top = _top; bottom = _bottom; left = _left; right = _right;
	}
};

class json_block {
	public:

	int tx;
	int ty;
	string name;
	string data;
	bool hascollision;
	bool isfullblock;
	map<string, interact_data> interactdata;

	json_block() {}

	json_block(int offx, int offy, string blockname, string blockdata, bool collision, bool fullblock, vector<json_interact_data> intdata) {
		tx = offx;
		ty = offy;
		name = blockname;
		data = blockdata;
		isfullblock = fullblock;
		hascollision = collision;
		for (int i = 0; i < intdata.size(); i++) {
			interactdata[intdata[i].name] = interact_data(intdata[i].top, intdata[i].bottom, intdata[i].left, intdata[i].right);
		}
	}
};

class world {
  public:
  
  block blocks[4096][256];
	int heightmap[4096];
	int worldseed = -1;

	world() {}

  void init(int seed) {
    PerlinNoise pn;
		worldseed = seed;

		// Basic terrain generation
    srand(seed);
		double randA = (double)(rand() % 256) / 16;
		double randB = (double)(rand() % 256) / 16;
		double randC = (double)(rand() % 256) / 16;
		for (int i = 0; i < 4096; i++) {
			for (int j = 0; j < 256; j++) {
				double px = (double)(i) / 256;
				double noiseA = pn.noise(px, randA, 16 - randB);
				double noiseB = pn.noise(px * 8, randB, 16 - randC);
				double noiseC = pn.noise(px * 32, randC, 16 - randA);
				double noiseT = (noiseA * 0.8 + noiseB * 0.15 + noiseC * 0.05);
				double noise = (noiseT * 128) + 96;
				blocks[i][j] = block((j > noise) ? "stone" : "air");
				if (blocks[i][j].type == "air" && j > 159) {
					blocks[i][j].waterlevel = 7;
				}
				blocks[i][j].wall = (j > noise) ? "stone-wall" : "empty";
			}
			if (i % 256 == 0) {
				cout << "Basic terrain generation: " << (i / 256) + 1 << " of 16 done." << endl;
			}
		}

		// Generating dirt, grass and sand
		for (int i = 0; i < 4096; i++) {
			double px = (double)(i) / 256;
			for (int j = 4; j < 256; j++) {
				if (blocks[i][j-1].type == "air" && blocks[i][j].type == "stone") {
					blocks[i][j].type = "dirt";
					blocks[i][j].wall = "dirt-wall";
				}
				if (blocks[i][j-2].type == "air" && blocks[i][j-1].type == "dirt" && blocks[i][j].type == "stone" && pn.noise((randB * randC) + (px * 8), 16 - randA, randB) >= 0.4) {
					blocks[i][j].type = "dirt";
					blocks[i][j].wall = "dirt-wall";
				}
				if (blocks[i][j-3].type == "air" && blocks[i][j-2].type == "dirt" && blocks[i][j-1].type == "dirt" && blocks[i][j].type == "stone" && pn.noise((randB * randC) + (px * 8), 16 - randA, randB) >= 0.4) {
					blocks[i][j].type = "dirt";
					blocks[i][j].wall = "dirt-wall";
				}
				if (blocks[i][j-4].type == "air" && blocks[i][j-3].type == "dirt" && blocks[i][j-2].type == "dirt" && blocks[i][j-1].type == "dirt" && blocks[i][j].type == "stone" && pn.noise((randB * randC) + (px * 8), 16 - randA, randB) >= 0.4) {
					blocks[i][j].type = "dirt";
					blocks[i][j].wall = "dirt-wall";
				}

				if (blocks[i][j-1].waterlevel > 0 && blocks[i][j].type == "stone") {
					if (pn.noise((randB * randC) + (px * 8), 16 - randA, randB) >= 0.4) {
						blocks[i][j].type = "sand";
						blocks[i][j].wall = "sand-wall";
					} else {
						blocks[i][j].type = "dirt";
						blocks[i][j].wall = "dirt-wall";
					}
				}

				if (blocks[i][j-2].waterlevel > 0 && blocks[i][j-1].type == "sand" && blocks[i][j].type == "stone") {
					if (pn.noise((randB * randC) + (px * 8), 16 - randA, randB) >= 0.2) {
						blocks[i][j].type = "sand";
						blocks[i][j].wall = "sand-wall";
					} else {
						blocks[i][j].type = "dirt";
						blocks[i][j].wall = "dirt-wall";
					}
				}

				if (blocks[i][j-3].waterlevel > 0 && blocks[i][j-2].type == "sand" && blocks[i][j-1].type == "sand" && blocks[i][j].type == "stone") {
					blocks[i][j].type = "sand";
				}
			}
			if (i % 256 == 0) {
				cout << "Dirt, grass and sand generation: " << (i / 256) + 1 << " of 16 done." << endl;
			}
		}

		// Generating caves (all at or under 191)
		for (int i = 0; i < 4096; i++) {
			for (int j = 0; j < 256; j++) {
				double px = (double)(i) / 32;
				double py = (double)(j) / 32;
				double a = (j - 139.0) / 58.0;
				double b = 0;
				if (a > 1) {
					b = a - 1;
					a = 1;
				}
				double cavefactor = ((0.70)*(1-a) + (0.55)*(a))*(1-b) + (0.85)*(b);
				if (blocks[i][j].type == "stone" || blocks[i][j].type == "dirt") {
					double perlin1 = pn.noise(px, py, (randA + randB) / (randC - randB));
					double perlin2 = pn.noise(px*2, py*2, (randB + randC) / (randA - randC));
					double perlin3 = pn.noise(px*4, py*4, (randC + randA) / (randB - randA));
					double perlin = (perlin1*0.8) + (perlin2*0.15) + (perlin3*0.05);
					if (perlin > cavefactor) {
						blocks[i][j].type = "air";
					}
				}
			}
			if (i % 256 == 0) {
				cout << "Cave generating: " << (i / 256) + 1 << " of 16 done." << endl;
			}
		}

		// Making trees:
		// - 1 tree trial every 8 blocks
		// - Randomly select between small, medium and large trees
		// - Random height
		for (int i2 = 0; i2 < 512; i2++) {
			int i = (i2*8) + 4;
			int j = 16;
			bool treemade = false;
			while (!treemade) {
				if (blocks[i][j].type == "air" && blocks[i][j].waterlevel == 0 && blocks[i][j+1].type == "dirt") {
					srand(seed*i - j);
					int r = rand()%4;
					if (r == 0) {
						int height = (rand()%10)+6;
							for (int k = (j - height) + 1; k <= j; k++) {
								blocks[i][k].type = "log";
								if (k % 2 == 0 && (k != (j - height) + 1 && k != j)) {
									int branch = rand()%8;
									if (branch == 0) {
										int length = (rand()%2)+1;
										for (int l = 1; l <= length; l++) {
											blocks[i+l][k].type = "log";
										}
									} else if (branch == 4) {
										int length = (rand()%2)+1;
										for (int l = 1; l <= length; l++) {
											blocks[i-l][k].type = "log";
										}
									}
								}
							}
							int cbsize = (rand()%3)+1;
							for (int k = -cbsize; k <= cbsize; k++) {
								for (int l = -cbsize; l <= cbsize; l++) {
									if (blocks[k+i][l+((j-height)+1)].type == "air") {
										blocks[k+i][l+((j-height)+1)].type = "leaves";
									}
								}
							}
							treemade = true;
					}
				}
				j++;
				treemade |= (j == 255);
			}
			if (i2 % 32 == 0) {
				cout << "Tree planting: " << (i2 / 32) + 1 << " of 16 done." << endl;
			}
		}

		// Placing minerals
		int lumpsplaced = 0;
		while (lumpsplaced < 1024) {
			int randx = 48+(rand()%4000);
			int randy = 128+(rand()%128);
			string lumptype = "stone";
			int lr = rand()%3;
			int maxlumpsize = 0;
			if (randy < 191) {
				if (lr == 1) {
					lr = 2;
				}
			}
			if (randy > 191) {
				if (lr == 2) {
					lr = 1;
				}
			}
			if (lr == 0) {
				lumptype = "iron";
				maxlumpsize = 4;
			} else if (lr == 1) {
				lumptype = "copper";
				maxlumpsize = 6;
			} else {
				lumptype = "tin";
				maxlumpsize = 8;
			}
			int lumpsize = (rand()%(maxlumpsize-1))+1;
			for (int i = -lumpsize; i <= lumpsize; i++) {
				int ti = tiabs(i);
				for (int j = -lumpsize; j <= lumpsize; j++) {
					int tj = tiabs(j);
					int rnd = rand()%lumpsize;
					if (blocks[randx+i][randy+j].type == "stone") {
						if (rnd >= ti) {
						  if (rnd >= tj) {
								blocks[randx+i][randy+j].type = lumptype;
							}
						}
					}
				}
			}
			lumpsplaced++;
		}

    // Folliage
    for (int i = 0; i < 4096; i++) {
			for (int j = 1; j < 256; j++) {
				if (blocks[i][j].type == "dirt") {
					if (blocks[i][j-1].type == "air") {
            bool placefolliage = false;
            if (j > 96) {
              placefolliage = rand()%8 == 0;
            } else {
              placefolliage = rand()%4 == 0;
            }
            if (placefolliage) {
              blocks[i][j-1].type = "grass";
            }
				  }
				}
			}
			if (i % 256 == 0) {
				cout << "Folliage: " << (i / 256) + 1 << " of 16 done." << endl;
			}
		}

		// Updating light
		for (int i = 0; i < 4096; i++) {
			for (int j = 0; j < 256; j++) {
				if (blocks[i][j].type == "air") {
					if (blocks[i][j].waterlevel <= 3) {
						addlight(i, j, 7);
					} else {
						addlight(i, j, 3);
					}
				} else {
					j = 256;
				}
			}
			if (i % 256 == 0) {
				cout << "Light updating: " << (i / 256) + 1 << " of 16 done." << endl;
			}
		}

		for (int i = 0; i < 4096; i++) {
			for (int j = 0; j < 256; j++) {
				if (blocks[i][j].type != "air") {
					heightmap[i] = j;
					j = 256;
				}
			}
		}
  }

	void addlight(int x, int y, int level) {
		blocks[x][y].lights.push_back(lightpoint(x, y, level));
		blocks[x][y].updatelight();
		for (int i = -level; i < level; i++) {
			for (int j = -level; j < level; j++) {
				int px = x + i;
				int py = y + j;
				if (px >= 0 && px <= 4095) {
					if (py >= 0 && py <= 255) {
						if (abs(i)+abs(j) <= level) {
							blocks[px][py].lights.push_back(lightpoint(x, y, level - (abs(i)+abs(j))));
							blocks[px][py].updatelight();
						}
					}
				}
			}
		}
	}
	void removelight(int x, int y) {
		for (int i = -7; i < 7; i++) {
			for (int j = -7; j < 7; j++) {
				int px = x + i;
				int py = y + j;
				if (px >= 0 && px <= 4095) {
					if (py >= 0 && py <= 255) {
						for (int k = 0; k < blocks[px][py].lights.size(); k++) {
							if (blocks[px][py].lights[k].x == x && blocks[px][py].lights[k].y == y) {
								blocks[px][py].eraselight(k);
								blocks[px][py].updatelight();
								k--;
							}
						}
					}
				}
			}
		}
	}

	void intenseupdatelight(int x, int y) {
		if (y == heightmap[x]) {
			if (blocks[x][y].type == "air") {
				bool domax = true;
				for (int i = y; i < 256; i++) {
					if (blocks[x][i].type == "air") {
						removelight(x, i);
						if (domax) {
							addlight(x, i, blocks[x][i].waterlevel <= 3 ? 7 : 3);
						} else {
							addlight(x, i, 3);
						}
						if (blocks[x][i].waterlevel > 3) {
							domax = false;
						}
					} else {
						heightmap[x] = i;
						i = 256;
					}
				}
			}
		}

		if (y < heightmap[x]) {
			if (blocks[x][y].type != "air") {
				for (int i = y; i <= heightmap[x]; i++) {
					removelight(x, i);
				}
				heightmap[x] = y;
			}
		}

		if (blocks[x][y].type == "air" && y > heightmap[x]) {
			if (blocks[x][y].lightlevel == 7) {
				removelight(x, y);
			}
		}
		blocks[x][y].updatelight();
	}

	void rintenseupdatelight(int x, int y) {
		if (sysmode == "server") {
			chunkbroadcast[x/32][y/32] = true;
		}
		if (blocks[x][y].lightlevel == 3 || blocks[x][y].lightlevel == 4) {
			intenseupdatelight(x, y);
		}
	}

	void tick(int x, int y) {
		if (blocks[x][y].waterlevel > 0) {
			if (y < 255) {
				if (blocks[x][y+1].waterlevel < 7) {
					if (blocks[x][y].type == "air" || blocks[x][y+1].type == "air") {
						int amt = imin(blocks[x][y].waterlevel, 7 - blocks[x][y+1].waterlevel);
						blocks[x][y].waterlevel -= amt;
						blocks[x][y+1].waterlevel += amt;
						rintenseupdatelight(x, y);
						rintenseupdatelight(x, y+1);
					}
				}
			}
			if (x > 0) {
				if (blocks[x][y].waterlevel > blocks[x-1][y].waterlevel && blocks[x-1][y].waterlevel < 7) {
					if (blocks[x][y].type == "air" || blocks[x-1][y].type == "air") {
						int amt = imin(blocks[x][y].waterlevel, 7 - blocks[x-1][y].waterlevel);
						amt = imax(1, amt/2);
						blocks[x][y].waterlevel -= amt;
						blocks[x-1][y].waterlevel += amt;
						rintenseupdatelight(x, y);
						rintenseupdatelight(x-1, y);
					}
				}
			}
			if (x < 4095) {
				if (blocks[x][y].waterlevel > blocks[x+1][y].waterlevel && blocks[x+1][y].waterlevel < 7) {
					if (blocks[x][y].type == "air" || blocks[x+1][y].type == "air") {
						int amt = imin(blocks[x][y].waterlevel, 7 - blocks[x+1][y].waterlevel);
						amt = imax(1, amt/2);
						blocks[x][y].waterlevel -= amt;
						blocks[x+1][y].waterlevel += amt;
						rintenseupdatelight(x, y);
						rintenseupdatelight(x+1, y);
					}
				}
			}
		}

		if (blocks[x][y].type == "dirt" && blocks[x][y].waterlevel > 4) {
			blocks[x][y].type = "mud";
			chunkbroadcast[x/32][y/32] = true;
		}
	}

	void saveat(map<string, unsigned short> blockmap, string dirpath) {
		cout << "Saving game..." << endl;
		vector<unsigned char> rawbytes;
		for (int i = 0; i < 4096; i++) {
			for (int j = 0; j < 256; j++) {
				rawbytes.push_back((unsigned char)(blockmap[blocks[i][j].type] % 256));
				rawbytes.push_back((unsigned char)(blockmap[blocks[i][j].type] / 256));
				rawbytes.push_back(0x00);
				rawbytes.push_back((unsigned char)(blocks[i][j].lightlevel + (blocks[i][j].waterlevel * 16)));
				rawbytes.push_back((unsigned char)(blocks[i][j].lights.size()));
				for (int k = 0; k < blocks[i][j].lights.size(); k++) {
					rawbytes.push_back((unsigned char)(blocks[i][j].lights[k].x % 256));
					rawbytes.push_back((unsigned char)(((blocks[i][j].lights[k].x / 256) % 16) + (blocks[i][j].lights[k].level * 16)));
					rawbytes.push_back((unsigned char)(blocks[i][j].lights[k].y));
				}
			}
		}
		cout << "Done!" << endl;
		std::experimental::filesystem::v1::create_directory(dirpath);
		ofstream worldfile(dirpath + "/world.bin", ios::out | ios::binary);
		worldfile.write((const char *)(&rawbytes[0]), rawbytes.size());
		worldfile.close();
	}

	void loadfrom(vector<json_block> blockids, string dirpath) {
		cout << "Opening file " << dirpath << "/world.bin..." << endl;
		long size = getfilesize(dirpath + "/world.bin");
		ifstream f(dirpath + "/world.bin", ios::in | ios::binary);
	 	unsigned char *rawbytes = (unsigned char *)malloc(size);
	 	if (f) {
			int c = 0;
			cout << "Reading file(1)..." << endl;
			f.read((char *)rawbytes, size);
			cout << "Reading file(2)..." << endl;
			for (int i = 0; i < 4096; i++) {
				for (int j = 0; j < 256; j++) {
					unsigned short ca = rawbytes[c+0];
					unsigned short cb = rawbytes[c+1];
					unsigned short cc = ca + (cb * 256);
					if ((int)(cc) != 65535) {
						blocks[i][j] = block(blockids[(int)(cc)].name);
					} else {
						blocks[i][j] = block("air");	
					}
					blocks[i][j].lightlevel = (int)(rawbytes[c+3]) % 16;
					blocks[i][j].waterlevel = (int)(rawbytes[c+3]) / 16;
					for (int k = 0; k < (int)(rawbytes[c+4]); k++) {
						unsigned short la = rawbytes[(c+5)+(k*3)];
						unsigned short lb = rawbytes[(c+5)+(k*3)+1];
						int lx = la | (lb << 8);
						blocks[i][j].lights.push_back(lightpoint(lx % 4096, (int)(rawbytes[(c+5)+(k*3)+2]), lx / 4096));
					}
					c = (c+5) + ((int)(rawbytes[c+4])*3);
				}
			}
			f.close();
			free(rawbytes);
			cout << "Done!" << endl;
	 	} else {
			cout << dirpath << "/world.bin not found. Closing the game." << endl;
			return;
		}

		for (int i = 0; i < 4096; i++) {
			for (int j = 0; j < 256; j++) {
				if (blocks[i][j].type != "air") {
					heightmap[i] = j;
					j = 256;
				}
			}
		}
	}
};
#endif