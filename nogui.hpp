#ifndef __NOGUI
#define __NOGUI
namespace nogui {
  vector<json_block> blockids;
	map<string, unsigned short> blockmap;

  string randstr(int len) {
    srand(time(NULL));
    string res = "";
    for (int i = 0; i < len; i++) {
      char c = (char)((rand() % 26) + 97);
      res += c;
    }
    if (DEBUG_MODE) {
      cout << "Random string done(" << res << ")" << endl;
    }
    return res;
  }

  void checkfordata() {
    if (DEBUG_MODE) {
      cout << "Making network checks..." << endl;
    }
		if (sysmode == "server") {
			while (enet_host_service(host, &event, check_time_ms_server) > 0) {
				switch (event.type) {
					case ENET_EVENT_TYPE_CONNECT: {
              if (DEBUG_MODE) {
                cout << "0.1" << endl;
              }
              event.peer->data = malloc(sizeof(player *));
              if (DEBUG_MODE) {
                cout << "0.2" << endl;
              }
              player *tmpp = new player({2048, 128}, "__" + randstr(12));
              if (DEBUG_MODE) {
                cout << "0.3" << endl;
              }
							cout << "<SERVER_INFO> Player " << tmpp->entitydata["name"] << " connected(" << event.peer->address.host << ":" << event.peer->address.port << ")" << endl;
							*(player **)(event.peer->data) = tmpp;
              if (DEBUG_MODE) {
                cout << "0.4" << endl;
              }
              int fspot = entityueid.size();
              if (ueidfrag > 0) {
                for (int i = 0; i < entityueid.size(); i++) {
                  if (entities[entityueid[i]]->entitydata["type"] == "placeholder") {
                    fspot = i;
                    i = entityueid.size();
                  }
                }
                ueidfrag--;
              }
              if (DEBUG_MODE) {
                cout << "0.5" << endl;
              }
							entities[tmpp->entitydata["name"]] = tmpp;
              if (fspot == entityueid.size()) {
                entityueid.push_back(tmpp->entitydata["name"]);
              } else {
                entityueid[fspot] = tmpp->entitydata["name"];
              }
              if (DEBUG_MODE) {
                cout << "0.6" << endl;
              }
							unsigned char *pktdata = (unsigned char *)malloc(1);
							pktdata[0] = (P_welcome * 16);
							ENetPacket *packet = enet_packet_create((void *)pktdata, 1, ENET_PACKET_FLAG_RELIABLE);
              if (DEBUG_MODE) {
                cout << "0.7" << endl;
              }
							enet_peer_send(event.peer, 0, packet);
							enet_host_flush(host);
							free(pktdata);
							playercnt++;
              if (DEBUG_MODE) {
                cout << "0.8" << endl;
              }
						}
						break;
					case ENET_EVENT_TYPE_RECEIVE: {
              if (DEBUG_MODE) {
                cout << "1" << endl;
              }
							string _player = (*(player **)(event.peer->data))->entitydata["name"];
							// Here we need to decode the packet
							unsigned char *pktdata = event.packet->data;
							int pkttype = pktdata[0] / 16;
              if (DEBUG_MODE) {
                cout << "2" << endl;
              }
							if (pkttype == P_chunk_request) {
								// Client requested a chunk
                if (DEBUG_MODE) {
                  cout << "3a" << endl;
                }
								int chunkx = pktdata[1];
								int chunky = pktdata[2];
								unsigned char *newpktdata = (unsigned char *)malloc(3+(32*32*4));
								newpktdata[0] = (P_chunk_data * 16);
								newpktdata[1] = chunkx;
								newpktdata[2] = chunky;
								int ind = 0;
								for (int y = 0; y < 32; y++) {
									for (int x = 0; x < 32; x++) {
										newpktdata[3+0+ind] = blockmap[w.blocks[(chunkx*32)+x][(chunky*32)+y].type] / 256;
										newpktdata[3+1+ind] = blockmap[w.blocks[(chunkx*32)+x][(chunky*32)+y].type] % 256;
										newpktdata[3+2+ind] = 0; // WALL
										newpktdata[3+3+ind] = w.blocks[(chunkx*32)+x][(chunky*32)+y].lightlevel + (w.blocks[(chunkx*32)+x][(chunky*32)+y].waterlevel * 16);
										ind += 4;
									}
								}
								ENetPacket *packet = enet_packet_create((void *)newpktdata, 3+(32*32*4), ENET_PACKET_FLAG_RELIABLE);
								enet_peer_send(event.peer, 0, packet);
								enet_host_flush(host);
								free(newpktdata);
							} else if (pkttype == P_update_player) {
                if (DEBUG_MODE) {
                  cout << "3b0" << endl;
                }
                unsigned char *ptx = (unsigned char *)malloc(sizeof(double));
                unsigned char *pty = (unsigned char *)malloc(sizeof(double));
                for (int i = 0; i < sizeof(double); i++) {
                  ptx[i] = pktdata[1+i];
                  pty[i] = pktdata[1+sizeof(double)+i];
                }
                if (DEBUG_MODE) {
                  cout << "3b1" << endl;
                }
                double px = *(double *)(ptx);
                double py = *(double *)(pty);
                entities[_player]->pos.x = px;
                entities[_player]->pos.y = py;
                if (DEBUG_MODE) {
                  cout << "3b2" << endl;
                }
                // Send player update packet here
                int sz = 1/*Packet info*/ + sizeof(double)/*X coord*/ + sizeof(double)/*Y coord*/ + 1/*Name length*/ + _player.size()/*Name*/;
                unsigned char *packetdata = (unsigned char *)malloc(sz);
                packetdata[0] = (P_update_player*16);
                if (DEBUG_MODE) {
                  cout << "3b3" << endl;
                }
                for (int i = 0; i < sizeof(double); i++) {
                  packetdata[1+i] = ptx[i];
                  packetdata[1+sizeof(double)+i] = pty[i];
                }
                packetdata[1+(2*sizeof(double))] = _player.size();
                for (int i = 0; i < _player.size(); i++) {
                  packetdata[1+(2*sizeof(double))+1+i] = _player.c_str()[i];
                }
                ENetPacket *packet = enet_packet_create((void *)packetdata, sz, ENET_PACKET_FLAG_RELIABLE);
                enet_host_broadcast(host, 0, packet);
                enet_host_flush(host);
                free(packetdata);
                free(ptx);
                free(pty);
							} else if (pkttype == P_place_block) {
                if (DEBUG_MODE) {
                  cout << "3c" << endl;
                }
								int bx = (256*(int)(pktdata[0]%16)) + (int)(pktdata[1]);
								int by = (int)(pktdata[2]);
								int bid = (256*(int)(pktdata[3])) + (int)(pktdata[4]);
								if (bid == 65535) {
									w.blocks[bx][by].type = "air";
								} else {
									w.blocks[bx][by].type = blockids[bid].name;
								}
								int chunkx = bx/32;
								int chunky = by/32;
								unsigned char *newpktdata = (unsigned char *)malloc(3+(32*32*4));
								newpktdata[0] = (P_chunk_data * 16);
								newpktdata[1] = chunkx;
								newpktdata[2] = chunky;
								int ind = 0;
								for (int y = 0; y < 32; y++) {
									for (int x = 0; x < 32; x++) {
										newpktdata[3+0+ind] = blockmap[w.blocks[(chunkx*32)+x][(chunky*32)+y].type] / 256;
										newpktdata[3+1+ind] = blockmap[w.blocks[(chunkx*32)+x][(chunky*32)+y].type] % 256;
										newpktdata[3+2+ind] = 0; // WALL
										newpktdata[3+3+ind] = w.blocks[(chunkx*32)+x][(chunky*32)+y].lightlevel + (w.blocks[(chunkx*32)+x][(chunky*32)+y].waterlevel * 16);
										ind += 4;
									}
								}
                w.intenseupdatelight(bx, by);
								ENetPacket *packet = enet_packet_create((void *)newpktdata, 3+(32*32*4), ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(host, 0, packet);
								enet_host_flush(host);
								free(newpktdata);
							} else if (P_client_new_player) {
                int len = pktdata[1];
                string newname = "";
                player *tmpp = *(player **)(event.peer->data);
                string oldname = tmpp->entitydata["name"];
                for (int i = 0; i < len; i++) {
                  newname += pktdata[2+i];
                }
                int ind = -1;
                for (int i = 0; i < entityueid.size(); i++) {
                  if (entityueid[i] == tmpp->entitydata["name"]) {
                    ind = i;
                    i = entityueid.size();
                  }
                }
                if (ind != -1) {
                  entityueid[ind] = newname;
                  tmpp->entitydata["name"] = newname;
                  entities.erase(oldname);
                  entities[newname] = tmpp;
                }
                _player = newname;
                cout << "<SERVER_INFO> Player " << oldname << " changed its name to " << newname << "." << endl;
                // Send packet here
                unsigned char *newpktdata = (unsigned char *)malloc(_player.size()+2);
                newpktdata[0] = (P_server_new_player*16);
                newpktdata[1] = _player.size();
                for (int i = 0; i < _player.size(); i++) {
                  newpktdata[2+i] = _player.c_str()[i];
                }
                ENetPacket *packet = enet_packet_create((void *)newpktdata, _player.size()+2, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(host, 0, packet);
								enet_host_flush(host);
								free(newpktdata);
              }
							enet_packet_destroy(event.packet);
						}
						break;
					case ENET_EVENT_TYPE_DISCONNECT:
            player *tmpp = *(player **)(event.peer->data);
            cout << "<SERVER_INFO> Player " << tmpp->entitydata["name"] << " disconnected." << endl;
            int ind = -1;
            for (int i = 0; i < entityueid.size(); i++) {
              if (entityueid[i] == tmpp->entitydata["name"]) {
                ind = i;
                i = entityueid.size();
              }
            }
            if (ind != -1) {
              // Send packet here
              string _player = string(tmpp->entitydata["name"]);
              unsigned char *newpktdata = (unsigned char *)malloc(_player.size()+2);
              newpktdata[0] = (P_player_left*16);
              newpktdata[1] = _player.size();
              for (int i = 0; i < _player.size(); i++) {
                newpktdata[2+i] = _player.c_str()[i];
              }
              ENetPacket *packet = enet_packet_create((void *)newpktdata, _player.size()+2, ENET_PACKET_FLAG_RELIABLE);
              enet_host_broadcast(host, 0, packet);
              enet_host_flush(host);
              entities.erase(tmpp->entitydata["name"]);
              entityueid.erase(entityueid.begin() + ind);
              free(newpktdata);
            }
            event.peer->data = NULL;
            playercnt--;
				}
			}
		}
	}

  std::chrono::_V2::system_clock::time_point last;
  std::chrono::_V2::system_clock::time_point last_update;

  void infloop() {
    auto start = std::chrono::high_resolution_clock::now();
    if (DEBUG_MODE) {
      cout << "Tick(1)!" << endl;
    }
    for (int i = 0; i < 128; i++) {
      for (int j = 0; j < 8; j++) {
        chunkbroadcast[i][j] = false;
      }
    }
    if (DEBUG_MODE) {
      cout << "Tick(2)!" << endl;
    }
    auto mid1 = std::chrono::high_resolution_clock::now();
    checkfordata();
    auto mid2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < entityueid.size(); i++) {
			if (entities[entityueid[i]]->entitydata["type"] == "player") {
				for (int x = -43; x < 43; x++) {
					for (int y = -24; y < 24; y++) {
						int cx = (entities[entityueid[i]]->pos.x+x)/32;
						int cy = (entities[entityueid[i]]->pos.y+y)/32;
						if (cx >= 0 && cx < 128) {
							if (cy >= 0 && cy < 8) {
								chunkbroadcast[cx][cy] = true;
							}
						}
					}
				}
			}
		}
    auto mid3 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_since_last_update = mid3 - last_update;
    if (time_since_last_update.count() > (1.0 / max_ticks_per_second)) {
      for (int i = 0; i < 128; i++) {
        for (int j = 0; j < 8; j++) {
          if (chunkbroadcast[i][j]/* && !chunkisair[i][j]*/) {
            for (int x = 0; x < 32; x++) {
              for (int y = 0; y < 32; y++) {
                w.tick(i*32 + x, j*32 + y);
                w.blocks[i*32 + x][j*32 + y].updatelight();
              }
            }
            int chunkx = i;
            int chunky = j;
            unsigned char *newpktdata = (unsigned char *)malloc(3+(32*32*4));
            newpktdata[0] = (P_chunk_data * 16);
            newpktdata[1] = chunkx;
            newpktdata[2] = chunky;
            int ind = 0;
            for (int y = 0; y < 32; y++) {
              for (int x = 0; x < 32; x++) {
                newpktdata[3+0+ind] = blockmap[w.blocks[(chunkx*32)+x][(chunky*32)+y].type] / 256;
                newpktdata[3+1+ind] = blockmap[w.blocks[(chunkx*32)+x][(chunky*32)+y].type] % 256;
                newpktdata[3+2+ind] = 0; // WALL
                newpktdata[3+3+ind] = w.blocks[(chunkx*32)+x][(chunky*32)+y].lightlevel + (w.blocks[(chunkx*32)+x][(chunky*32)+y].waterlevel * 16);
                ind += 4;
              }
            }
            ENetPacket *packet = enet_packet_create((void *)newpktdata, 3+(32*32*4), ENET_PACKET_FLAG_RELIABLE);
            enet_host_broadcast(host, 0, packet);
            enet_host_flush(host);
            free(newpktdata);
            chunkbroadcast[i][j] = false;
          }
        }
      }
      last_update = std::chrono::high_resolution_clock::now();
    }
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dbgtime = finish - last;
    if (dbgtime.count() > 1) {
      last = std::chrono::high_resolution_clock::now();
#ifdef _WIN32
      system("cls");
#else
      system("clear");
#endif
      std::chrono::duration<double> elapsed = finish - start;
      std::chrono::duration<double> elmid1 = mid1 - start;
      std::chrono::duration<double> elmid2 = mid2 - mid1;
      std::chrono::duration<double> elmid3 = mid3 - mid2;
      std::chrono::duration<double> elmid4 = finish - mid3;
      double dmid1 = elmid1.count() / elapsed.count();
      double dmid2 = elmid2.count() / elapsed.count();
      double dmid3 = elmid3.count() / elapsed.count();
      double dmid4 = elmid4.count() / elapsed.count();
      cout << "Server data: " << endl;
      cout << " - General information: " << endl;
      cout << "    - FPS: " << (1.0 / elapsed.count()) << " FPS" << endl;
      cout << "    - Tick length: " << (elapsed.count() * 1000.0) << " ms" << endl;
      cout << "    - Players: " << playercnt << endl;
      cout << " - Server CPU usage: " << endl;
      cout << "    - Cleaning: " << floor(dmid1*100.0) << " %, " << (elmid1.count() * 1000.0) << " ms" << endl;
      cout << "    - Networking: " << floor(dmid2*100.0) << " %, " << (elmid2.count() * 1000.0) << " ms" << endl;
      cout << "    - Chunk checking: " << floor(dmid3*100.0) << " %, " << (elmid3.count() * 1000.0) << " ms" << endl;
      cout << "    - Chunk updating: " << floor(dmid4*100.0) << " %, " << (elmid4.count() * 1000.0) << " ms" << endl;
      cout << "Players: " << endl;
      for (int i = 0; i < entityueid.size(); i++) {
        if (entities[entityueid[i]]->entitydata["type"] == "player") {
          cout << " - " << "\"" << entities[entityueid[i]]->entitydata["name"] << "\", located at (" << entities[entityueid[i]]->pos.x << ", " << entities[entityueid[i]]->pos.y << ")" << endl;
        }
      }
    }
  }

  void init() {
    if (DEBUG_MODE) {
      cout << "Initializating server(0 of 3)..." << endl;
    }
    ifstream f("game/packs/default/blocklist.json");
	 	string blocklist;
	 	if (f) {
			ostringstream ss;
			ss << f.rdbuf();
			blocklist = ss.str();
	 	} else {
			cout << "game/packs/default/blocklist.json not found. Closing the game." << endl;
			return;
		}
    if (DEBUG_MODE) {
      cout << "Initializating server(1 of 3)..." << endl;
    }
		auto blockjson = json::parse(blocklist);
		int blockcnt = blockjson["blocks"].size();
		for (int i = 0; i < blockcnt; i++) {
			vector<json_interact_data> tmp_interact_state;
			for (int j = 0; j < blockjson["blocks"][i]["interact-data"].size(); j++) {
				tmp_interact_state.push_back(json_interact_data(blockjson["blocks"][i]["interact-data"][j]["name"].get<string>(), blockjson["blocks"][i]["interact-data"][j]["sides"][0].get<bool>(), blockjson["blocks"][i]["interact-data"][j]["sides"][1].get<bool>(), blockjson["blocks"][i]["interact-data"][j]["sides"][2].get<bool>(), blockjson["blocks"][i]["interact-data"][j]["sides"][3].get<bool>()));
			} 
			blockids.push_back(json_block(blockjson["blocks"][i]["texture"][0].get<int>(), blockjson["blocks"][i]["texture"][1].get<int>(), blockjson["blocks"][i]["name"].get<string>(), blockjson["blocks"][i].dump(), blockjson["blocks"][i]["has-collision"].get<bool>(), blockjson["blocks"][i]["is-full-block"].get<bool>(), tmp_interact_state));
			blockmap[blockjson["blocks"][i]["name"].get<string>()] = (unsigned short)(i);
		}
    if (DEBUG_MODE) {
      cout << "Initializating server(2 of 3)..." << endl;
    }
		for (int i = 0; i < blockcnt; i++) {
			for (int j = 0; j < blockids.size(); j++) {
				if (blockids[i].interactdata.find(blockids[j].name) == blockids[i].interactdata.end()) {
					if (i == j) {
						blockids[i].interactdata[blockids[i].name] = interact_data(true, true, true, true);
					} else {
						blockids[i].interactdata[blockids[j].name] = interact_data(blockjson["blocks"][i]["interact-default-data"][0].get<bool>(), blockjson["blocks"][i]["interact-default-data"][1].get<bool>(), blockjson["blocks"][i]["interact-default-data"][2].get<bool>(), blockjson["blocks"][i]["interact-default-data"][3].get<bool>());
					}
				}
			}
			blockids[i].interactdata["air"] = interact_data(false, false, false, false);
		}
    if (DEBUG_MODE) {
      cout << "Initializating server(3 of 3)..." << endl;
    }
		blockmap["air"] = 65535;
    if (fileexists(makewincompatible("game/worlds/" + worldname))) {
      cout << "World already generated. Using world '" << worldname << "'." << endl;
      w.loadfrom(blockids, makewincompatible("game/worlds/" + worldname));
    } else {
      int worldseed;
      cout << "Seed for new world(0 to 2147483647): ";
      cin >> worldseed;
      cout << "Generating new world with seed '" << worldseed << "'." << endl;
      w.init(worldseed);
      w.saveat(blockmap, makewincompatible("game/worlds/" + worldname));
    }
    cout << "Server started!" << endl;
    last = std::chrono::high_resolution_clock::now();
    while (true) {
      infloop();
    }
  }
}
#endif