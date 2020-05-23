#define OLC_PGE_APPLICATION
#include "libs/olcPixelGameEngine.h"
#include "libs/olcPGEX_Sound.h"
#include "libs/json.hpp"

#define DEBUG_MODE false

#define max_ticks_per_second 10
#define check_time_ms_server 0
#define check_time_ms_client 0

//#ifdef _WIN32
#include "libs/enet/enet/enet.h"
//#else
//#include <enet/enet.h>
//#endif

// Client to Server
#define P_place_block 0
#define P_chunk_request 1
#define P_update_request 2
#define P_client_new_player 3

// Both
#define P_update_player 4

// Server to Client
#define P_player_left 12
#define P_server_new_player 13
#define P_welcome 14
#define P_chunk_data 15

using json = nlohmann::json;
using namespace std;

string currplayer = "player";

string sysmode;

string worldname;

ENetAddress address;
ENetHost *host;
ENetEvent event;
ENetPeer *peer;

bool chunkloaded[128][8];
bool chunkrequest[128][8];
bool chunkbroadcast[128][8];

float daytime = 4; // Measured in game hours (day = 24 hours), but 1 hour is 1 real-life minute
float timeweatherupdate = 0; // Random between 10 and 400 (in seconds)

// From hour when it starts to hour when it ends(beggining of the hour).
// Gradients are 3/4 of the first color and 1/4 of the second, and move at half the Y axis, to give a parallax effect.

#define H_04_TO_07_RED_1   52
#define H_04_TO_07_GREEN_1 154
#define H_04_TO_07_BLUE_1  255
#define H_04_TO_07_RED_2   192
#define H_04_TO_07_GREEN_2 146
#define H_04_TO_07_BLUE_2  255

#define H_07_TO_10_RED_1   86
#define H_07_TO_10_GREEN_1 191
#define H_07_TO_10_BLUE_1  255
#define H_07_TO_10_RED_2   180
#define H_07_TO_10_GREEN_2 207
#define H_07_TO_10_BLUE_2  228

#define H_10_TO_13_RED_1   115
#define H_10_TO_13_GREEN_1 244
#define H_10_TO_13_BLUE_1  255
#define H_10_TO_13_RED_2   169
#define H_10_TO_13_GREEN_2 237
#define H_10_TO_13_BLUE_2  255

float dayred1 = H_04_TO_07_RED_1;
float daygreen1 = H_04_TO_07_GREEN_1;
float dayblue1 = H_04_TO_07_BLUE_1;
float dayred2 = H_04_TO_07_RED_2;
float daygreen2 = H_04_TO_07_GREEN_2;
float dayblue2 = H_04_TO_07_BLUE_2;

float sdayred1 = H_04_TO_07_RED_1;
float sdaygreen1 = H_04_TO_07_GREEN_1;
float sdayblue1 = H_04_TO_07_BLUE_1;
float sdayred2 = H_04_TO_07_RED_2;
float sdaygreen2 = H_04_TO_07_GREEN_2;
float sdayblue2 = H_04_TO_07_BLUE_2;

void selectdaycolors() {
  if (daytime >= 4 && daytime < 7) {
    dayred1 = H_04_TO_07_RED_1;
    daygreen1 = H_04_TO_07_GREEN_1;
    dayblue1 = H_04_TO_07_BLUE_1;
    dayred2 = H_04_TO_07_RED_2;
    daygreen2 = H_04_TO_07_GREEN_2;
    dayblue2 = H_04_TO_07_BLUE_2;
  } else if (daytime >= 7 && daytime < 10) {
    dayred1 = H_07_TO_10_RED_1;
    daygreen1 = H_07_TO_10_GREEN_1;
    dayblue1 = H_07_TO_10_BLUE_1;
    dayred2 = H_07_TO_10_RED_2;
    daygreen2 = H_07_TO_10_GREEN_2;
    dayblue2 = H_07_TO_10_BLUE_2;
  } else if (daytime >= 10 && daytime < 13) {
    dayred1 = H_10_TO_13_RED_1;
    daygreen1 = H_10_TO_13_GREEN_1;
    dayblue1 = H_10_TO_13_BLUE_1;
    dayred2 = H_10_TO_13_RED_2;
    daygreen2 = H_10_TO_13_GREEN_2;
    dayblue2 = H_10_TO_13_BLUE_2;
  }
  sdayred1 = (sdayred1*0.975) + (dayred1*0.025);
  sdaygreen1 = (sdaygreen1*0.975) + (daygreen1*0.025);
  sdayblue1 = (sdayblue1*0.975) + (dayblue1*0.025);
  sdayred2 = (sdayred2*0.975) + (dayred2*0.025);
  sdaygreen2 = (sdaygreen2*0.975) + (daygreen2*0.025);
  sdayblue2 = (sdayblue2*0.975) + (dayblue2*0.025);
}

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#ifdef _WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

string makewincompatible(string path) {
#ifdef _WIN32
  #warning Changing all occurences of / in paths with \...
  string newpath = "";
  char cCurrentPath[FILENAME_MAX];
  int bytes = GetModuleFileName(NULL, cCurrentPath, FILENAME_MAX);
  newpath += cCurrentPath;
  newpath = newpath.substr(0, newpath.find_last_of("/\\"));
  newpath += "\\";
  for (int i = 0; i < path.size(); i++) {
    if (path.c_str()[i] == '/') {
      newpath += '\\';
    } else {
      newpath += path.c_str()[i];
    }
  }
  cout << newpath << endl;
  return newpath;
#else
  return path;
#endif
}

#include "world.hpp"
#include "block.hpp"
#include "entity.hpp"
#include "player.hpp"

world w;

bool fileexists(string path) {
  return std::experimental::filesystem::exists(path);
}

int playercnt = 0;

#include "nogui.hpp"

float invscroll = 0;
float invscrollfinal = 0;

float scrolltransparency = 0;

float loadticks = 0;

#define LOADING_SCENE_1 1
#define LOADING_SCENE_2 2
#define MENU_SCENE      3
#define GAME_SCENE      4

int scene = LOADING_SCENE_1;

class JustSquares : public olc::PixelGameEngine {
  public:

  JustSquares() {
    sAppName = "Just Squares";
  }

  vector<json_block> blockids;

  map<string, unsigned short> blockmap;

  olc::Decal *texture = nullptr;
  olc::Decal *watert = nullptr;
  olc::Sprite *textureimage = nullptr;
  olc::Sprite *watertimage = nullptr;

  olc::Decal *invslot = nullptr;
  olc::Sprite *invslotimage = nullptr;

  olc::Decal *selslot = nullptr;
  olc::Sprite *selslotimage = nullptr;

  olc::Sprite *tmpimage = nullptr;
  olc::Decal *tmp = nullptr;

  olc::Sprite *olcpgeimage = nullptr;
  olc::Decal *olcpge = nullptr;

  string playmode = "moving";

  std::chrono::_V2::system_clock::time_point last;

  void checkfordata() {
    if (sysmode == "client") {
      while (enet_host_service(host, &event, check_time_ms_client) > 0) {
        switch (event.type) {
          case ENET_EVENT_TYPE_CONNECT:
            cout << "<CLIENT_INFO> Conneted to server succefully." << endl;
            break;
          case ENET_EVENT_TYPE_RECEIVE: {
              // Here we need to decode the packet
              unsigned char *pktdata = event.packet->data;
              int pkttype = pktdata[0] / 16;
              if (pkttype == P_chunk_data) {
                // We have received a chunk
                int chunkx = pktdata[1];
                int chunky = pktdata[2];
                if (chunkloaded[chunkx][chunky]) {
                  int ind = 0;
                  for (int y = 0; y < 32; y++) {
                    for (int x = 0; x < 32; x++) {
                      if (((int)(pktdata[3+ind]) * 256) + (int)(pktdata[4+ind]) == 65535) {
                        w.blocks[(chunkx*32)+x][(chunky*32)+y].type = "air";
                      } else {
                        w.blocks[(chunkx*32)+x][(chunky*32)+y].type = blockids[((int)(pktdata[3+ind]) * 256) + (int)(pktdata[4+ind])].name;
                      }
                      w.blocks[(chunkx*32)+x][(chunky*32)+y].wall = ""/*pktdata[5+ind]*/;
                      w.blocks[(chunkx*32)+x][(chunky*32)+y].lightlevel = pktdata[6+ind] % 16;
                      w.blocks[(chunkx*32)+x][(chunky*32)+y].waterlevel = pktdata[6+ind] / 16;
                      ind += 4;
                    }
                  }
                } else {
                  int ind = 0; 
                  for (int y = 0; y < 32; y++) {
                    for (int x = 0; x < 32; x++) {
                      if (((int)(pktdata[3+ind]) * 256) + (int)(pktdata[4+ind]) == 65535) {
                        w.blocks[(chunkx*32)+x][(chunky*32)+y] = block("air");
                      } else {
                        w.blocks[(chunkx*32)+x][(chunky*32)+y] = block(blockids[((int)(pktdata[3+ind]) * 256) + (int)(pktdata[4+ind])].name);
                      }
                      w.blocks[(chunkx*32)+x][(chunky*32)+y].wall = "";
                      w.blocks[(chunkx*32)+x][(chunky*32)+y].lightlevel = pktdata[6+ind] % 16;
                      w.blocks[(chunkx*32)+x][(chunky*32)+y].waterlevel = pktdata[6+ind] / 16;
                      ind += 4;
                    }
                  }
                }
                chunkloaded[chunkx][chunky] = true;
                chunkrequest[chunkx][chunky] = false;
              } else if (pkttype == P_welcome) {
                // Send packet with name
                int sz = 1/*Packet info*/ + 1/*Name length*/ + currplayer.size()/*Name*/;
                unsigned char *packetdata = (unsigned char *)malloc(sz);
                packetdata[0] = (P_client_new_player*16);
                packetdata[1] = currplayer.size();
                for (int i = 0; i < currplayer.size(); i++) {
                  packetdata[2+i] = currplayer.c_str()[i];
                }
                ENetPacket *packet = enet_packet_create((void *)packetdata, sz, ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(peer, 0, packet);
                enet_host_flush(host);
                free(packetdata);
              } else if (pkttype == P_server_new_player) {
                // Add player to entity list
                string pname = "";
                for (int i = 0; i < pktdata[1]; i++) {
                  pname += pktdata[2+i];
                }
                entityueid.push_back(pname);
                entities[pname] = new player({2048, 128}, pname);
                cout << "Player " << pname << " joined." << endl;
              } else if (pkttype == P_player_left) {
                // Remove player from entity list
                string pname = "";
                for (int i = 0; i < pktdata[1]; i++) {
                  pname += pktdata[2+i];
                }
                int fspot = -1;
                for (int i = 0; i < entityueid.size(); i++) {
                  if (entityueid[i] == pname) {
                    fspot = i;
                    i = entityueid.size();
                  }
                }
                if (fspot != -1) {
                  entityueid.erase(entityueid.begin() + fspot);
                  entities.erase(pname);
                  playercnt--;
                  cout << "Player " << pname << " disconnected." << endl;
                }
              } else if (pkttype == P_update_player) {
                // We have received a player data update
                string pname = "";
                for (int i = 0; i < pktdata[1+(2*sizeof(double))]; i++) {
                  pname += pktdata[1+(2*sizeof(double))+1+i];
                }
                unsigned char *ptx = (unsigned char *)malloc(sizeof(double));
                unsigned char *pty = (unsigned char *)malloc(sizeof(double));
                for (int i = 0; i < sizeof(double); i++) {
                  ptx[i] = pktdata[1+i];
                  pty[i] = pktdata[1+sizeof(double)+i];
                }
                double px = *(double *)(ptx);
                double py = *(double *)(pty);
                entities[pname]->pos.x = px;
                entities[pname]->pos.y = py;
                free(ptx);
                free(pty);
              }
              enet_packet_destroy(event.packet);
            }
            break;
          case ENET_EVENT_TYPE_DISCONNECT:
            cout << "<CLIENT_WARN> Connection finished, either timed out or server closed." << endl;
        }
      }
      for (int x = 0; x < 128; x++) {
        for (int y = 0; y < 8; y++) {
          if (!chunkloaded[x][y]) {
            if (!chunkrequest[x][y]) {
              chunkrequest[x][y] = true;
              int sz = 1/*Packet info*/ + 1/*X coord*/ + 1/*Y coord*/;
              unsigned char *packetdata = (unsigned char *)malloc(sz);
              packetdata[0] = (P_chunk_request * 16);
              packetdata[1] = x;
              packetdata[2] = y;
              ENetPacket *packet = enet_packet_create((void *)packetdata, sz, ENET_PACKET_FLAG_RELIABLE);
              enet_peer_send(peer, 0, packet);
              enet_host_flush(host);
              free(packetdata);
              last = std::chrono::high_resolution_clock::now();
            } else {
              auto current = std::chrono::high_resolution_clock::now();
              std::chrono::duration<double> time_since_last_request = current - last;
              if (time_since_last_request.count() > 1000) {
                chunkrequest[x][y] = false;
                last = std::chrono::high_resolution_clock::now();
              }
            }
            return;
          }
        }
      }
    }
  }

  int guilayer;
  int backgroundlayer;

  bool OnUserCreate() override {
    if (DEBUG_MODE) {
      cout << "Loading game(0 of 4)..." << endl;
    }
    entities[currplayer] = new player({2048, 128}, currplayer);
    entityueid.push_back(currplayer);
    playercnt++;
    tmpimage = new olc::Sprite(1, 1);
    tmpimage->SetPixel({0, 0}, olc::Pixel(0, 0, 0));
    tmp = new olc::Decal(tmpimage);
    invslotimage = new olc::Sprite(makewincompatible("game/internal/textures/inventory-slot.png"));
    invslot = new olc::Decal(invslotimage);
    selslotimage = new olc::Sprite(makewincompatible("game/internal/textures/selected-slot.png"));
    selslot = new olc::Decal(selslotimage);
    if (DEBUG_MODE) {
      cout << "Loading game(1 of 4)..." << endl;
    }
    ifstream f("game/packs/default/blocklist.json");
     string blocklist;
     if (f) {
      ostringstream ss;
      ss << f.rdbuf();
      blocklist = ss.str();
     } else {
      cout << "game/packs/default/blocklist.json not found. Closing the game." << endl;
      return false;
    }
    if (DEBUG_MODE) {
      cout << "Loading game(2 of 4)..." << endl;
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
      cout << "Loading game(3 of 4)..." << endl;
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
      cout << "Loading game(4 of 4)..." << endl;
    }
    blockmap["air"] = 65535;
    
    textureimage = new olc::Sprite(makewincompatible("game/packs/default/textures/blocks.png"));
    watertimage = new olc::Sprite(makewincompatible("game/internal/textures/water.png"));
    texture = new olc::Decal(textureimage);
    watert = new olc::Decal(watertimage);
    if (sysmode == "server" || sysmode == "offline") {
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
    }
    if (DEBUG_MODE) {
      cout << "Loading splashscreen..." << endl;
    }
    olcpgeimage = new olc::Sprite(makewincompatible("game/internal/splashscreen/olc-pge-logo.png"));
    olcpge = new olc::Decal(olcpgeimage);
    if (DEBUG_MODE) {
      cout << "Done loading game!" << endl;
    }
    //guilayer = 0;
    //backgroundlayer = CreateLayer();
    //EnableLayer(backgroundlayer, true);
    //SetDrawTarget(backgroundlayer);
    return true;
  }

  bool OnUserUpdate(float fElapsedTime) override {
    if (scene == LOADING_SCENE_1) {
      Clear(olc::Pixel(255, 255, 255));
      // Made using olc::PGE
      float val = 96 * (4 - abs(loadticks - 4));
      if (val > 255) val = 255;
      if (val < 0) val = 0;
      DrawDecal({128.0f, 148.0f}, olcpge, {0.5f, 0.5f}, olc::Pixel(255, 255, 255, val));
      if (loadticks >= 8) {
        scene = LOADING_SCENE_2;
        loadticks = 0;
      }
    } else if (scene == LOADING_SCENE_2) {
      Clear(olc::Pixel(255, 255, 255));
      // segfaultcoder
      float val = 72 * (4 - abs(loadticks - 4));
      if (val > 255) val = 255;
      if (val < 0) val = 0;
      DrawDecal({128.0f, 148.0f}, olcpge, {0.5f, 0.5f}, olc::Pixel(255, 255, 255, val));
      if (loadticks >= 8) {
        scene = GAME_SCENE;
        loadticks = 0;
      }
    } else if (scene == MENU_SCENE) {
      Clear(olc::Pixel(82, 194, 255));
      // Main menu
    } else if (scene == GAME_SCENE) {
      if (DEBUG_MODE) {
        cout << "Tick(0 of 4)!" << endl;
      }
      if (sysmode == "client") {
        if (DEBUG_MODE) {
          cout << "Checking for network updates..." << endl;
        }
        checkfordata();
      }
      if (DEBUG_MODE) {
        cout << "Tick(1 of 4)!" << endl;
      }
      Clear(olc::Pixel(sdayred1, sdaygreen1, sdayblue1));
      int iposx = (int)(floor(entities[currplayer]->pos.x));
      int iposy = (int)(floor(entities[currplayer]->pos.y));
      double sposx = entities[currplayer]->pos.x;
      double sposy = entities[currplayer]->pos.y;
      int mx = (int)(floor(((GetMouseX() - (320-4)) / 8.0) + (sposx - (double) (iposx))));
      int my = (int)(floor(((GetMouseY() - (180-8)) / 8.0) + (sposy - (double) (iposy))));
      if (chunkloaded[(mx+iposx)/32][(my+iposy)/32]) {
        if (GetMouse(0).bHeld &&	w.blocks[mx+iposx][my+iposy].type == "air" && inventory[currinvslot][0].name != "air") {
          if (sysmode == "client") {
            int sz = 1/*Packet info & part of X coord*/ + 2/*End of X coord and Y coord*/ + 2/*Block type id*/;
            unsigned char *packetdata = (unsigned char *)malloc(sz);
            packetdata[0] = (P_place_block * 16) | (((mx)+iposx) / 256);
            packetdata[1] = ((mx)+iposx) % 256;
            packetdata[2] = ((my)+iposy);
            int tdd = blockmap["dirt"];
            packetdata[3] = tdd / 256;
            packetdata[4] = tdd % 256;
            ENetPacket *packet = enet_packet_create((void *)packetdata, sz, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(peer, 0, packet);
            enet_host_flush(host);
            free(packetdata);
          } else {
            w.blocks[mx+iposx][my+iposy].type = inventory[currinvslot][0].name;
            takefrominv(inventory[currinvslot][0].name, 1, currinvslot);
            w.intenseupdatelight(mx+iposx, my+iposy);
          }
        } else if (GetMouse(1).bHeld &&	w.blocks[mx+iposx][my+iposy].type != "air") {
          if (sysmode == "client") {
            int sz = 1/*Packet info & part of X coord*/ + 2/*End of X coord and Y coord*/ + 2/*Block type id*/;
            unsigned char *packetdata = (unsigned char *)malloc(sz);
            packetdata[0] = (P_place_block * 16) | (((mx)+iposx) / 256);
            packetdata[1] = ((mx)+iposx) % 256;
            packetdata[2] = ((my)+iposy);
            int tdd = 65535;
            packetdata[3] = tdd / 256;
            packetdata[4] = tdd % 256;
            ENetPacket *packet = enet_packet_create((void *)packetdata, sz, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(peer, 0, packet);
            enet_host_flush(host);
            free(packetdata);
          } else {
            pushtoinv(w.blocks[mx+iposx][my+iposy].type, 1);
            w.blocks[mx+iposx][my+iposy].type = "air";
            w.intenseupdatelight(mx+iposx, my+iposy);
          }
        } else if (GetMouse(2).bHeld && (sysmode == "offline")) {
          w.blocks[(mx-43)+iposx][(my-24)+iposy].waterlevel = 7;
          w.intenseupdatelight((mx-43)+iposx, (my-24)+iposy);
        }
      }
      if (DEBUG_MODE) {
        cout << "Tick(2 of 4)!" << endl;
      }
      //cout << entities.size() << endl;
      for (int i = 0; i < entityueid.size(); i++) {
        olc::Decal *d = entities[entityueid[i]]->ondraw();
        if (d != nullptr) {
          if (entityueid[i] == currplayer) {
            DrawDecal({320-8, 180-16}, d);
          } else {
            DrawDecal({(entities[entityueid[i]]->pos.x - entities[currplayer]->pos.x)+(320-8), (entities[entityueid[i]]->pos.y - entities[currplayer]->pos.y)+(180-16)}, d);
          }
        }
      }
      if (DEBUG_MODE) {
        cout << "Tick(3 of 4)!" << endl;
      }
      if (sysmode == "server") {
        for (int i = 0; i < 128; i++) {
          for (int j = 0; j < 8; j++) {
            if (chunkbroadcast[i][j]) {
              for (int x = 0; x < 32; x++) {
                for (int y = 0; y < 32; y++) {
                  w.tick(i*32 + x, j*32 + y);
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
      }
      if (DEBUG_MODE) {
        cout << "Tick(4 of 4)!" << endl;
      }
      for (int i = -43; i < 43; i++) {
        for (int j = 28; j >= -28; j--) {
          if (i+iposx >= 0 && i+iposx <= 4095) {
            if (j+iposy >= 0 && j+iposy <= 255) { 
              if (chunkloaded[(i+iposx)/32][(j+iposy)/32]) {
                if (sysmode == "server" || sysmode == "offline") {
                  w.blocks[i+iposx][j+iposy].updatelight();
                }
                int blockstyle = 0;
                string btype = w.blocks[i+iposx][j+iposy].type;
                if (w.blocks[i+iposx][j+iposy].type != "air") {
                  if (j+iposy > 0) {
                    if (!(blockids[blockmap[w.blocks[i+iposx][j+iposy].type]].interactdata[w.blocks[i+iposx][j+iposy-1].type].top)) {
                      blockstyle |= 1;
                    }
                  }
                  if (j+iposy < 255) {
                    if (!(blockids[blockmap[w.blocks[i+iposx][j+iposy].type]].interactdata[w.blocks[i+iposx][j+iposy+1].type].bottom)) {
                      blockstyle |= 2;
                    }
                  }

                  if (i+iposx > 0) {
                    if (!(blockids[blockmap[w.blocks[i+iposx][j+iposy].type]].interactdata[w.blocks[i+iposx-1][j+iposy].type].left)) {
                      blockstyle |= 4;
                    }
                  }
                  if (i+iposx < 4095) {
                    if (!(blockids[blockmap[w.blocks[i+iposx][j+iposy].type]].interactdata[w.blocks[i+iposx+1][j+iposy].type].right)) {
                      blockstyle |= 8;
                    }
                  }
                }
                int supxs[] = {1, 1, 1, 3, 0, 0, 0, 3, 2, 2, 2, 5, 5, 4, 4, 4};
                int supys[] = {1, 0, 2, 0, 1, 0, 2, 1, 1, 0, 2, 1, 0, 0, 2, 1};
                int supx = supxs[blockstyle];
                int supy = supys[blockstyle];
                olc::vf2d scrpos = {((i) - (sposx - (double) (iposx)))*8.0 + (320 - 4), ((j)-(sposy - (double)(iposy)))*8.0 + (180 - 8)};
                olc::vf2d relpos;
                if (w.blocks[i+iposx][j+iposy].type == "air") {
                  relpos = {0, 0};
                } else {
                  relpos = {(supx*8) + (blockids[blockmap[w.blocks[i+iposx][j+iposy].type]].tx * 48.0), (supy*8) + (blockids[blockmap[w.blocks[i+iposx][j+iposy].type]].ty * 24.0)};
                }
                olc::vf2d rlsize = {8.0, 8.0};
                //if (i < 81 && i >= -1) {
                  if (w.blocks[i+iposx][j+iposy].waterlevel > 0) {
                    if (j+iposy > 0) {
                      if (w.blocks[i+iposx][(j+iposy)-1].waterlevel == 0) {
                        olc::vf2d waterpos = {scrpos.x, scrpos.y + (8 - w.blocks[i+iposx][j+iposy].waterlevel)};
                        DrawDecal(waterpos, watert, {1.0, 0.125*w.blocks[i+iposx][j+iposy].waterlevel}, olc::Pixel(255, 255, 255, 127));
                      } else {
                        DrawDecal(scrpos, watert, {1.0, 1.0}, olc::Pixel(255, 255, 255, 127));
                      }
                    } else {
                      DrawDecal(scrpos, watert, {1.0, 1.0}, olc::Pixel(255, 255, 255, 127));
                    }
                  }
                  if (w.blocks[i+iposx][j+iposy].type != "air") {
                    int lightvalue = (32*w.blocks[i+iposx][j+iposy].lightlevel);
                    DrawPartialDecal(scrpos, texture, relpos, rlsize, {1.0, 1.0}, olc::Pixel(lightvalue, lightvalue, lightvalue));
                  }
                //}
                if (sysmode == "offline") {
                  w.tick(i+iposx, j+iposy);
                }
              }
            }
          }
        }
      }
      double pwidth = 0.8;
      double pheight = 1.8;
      int tposx = floor(entities[currplayer]->pos.x);
      int tposy = floor(entities[currplayer]->pos.y);
      for (int i = -3; i < 3; i++) {
        for (int j = -3; j < 3; j++) {
          double x = i+tposx;
          double y = j+tposy;
          double x1 = entities[currplayer]->pos.x - (pwidth/2);
          double y1 = entities[currplayer]->pos.y - (pheight/2);
          if (w.blocks[(int)(x)][(int)(y)].type != "air") {
            if (blockids[blockmap[w.blocks[(int)(x)][(int)(y)].type]].hascollision) {
              if (inrects(x1, y1, x1+pwidth, y1+pheight, x, y, x+1, y+1)) {
                double ang = round(atan2(y1-y, x1-x) / (PI/2))*(PI/2);
                vec2d p1 = intersect(x+0.5, y+0.5, x1+(pwidth/2), y1+(pheight/2), x1, y1+pheight, x1+pwidth, y1+pheight);
                vec2d p2 = intersect(x+0.5, y+0.5, x1+(pwidth/2), y1+(pheight/2), x1, y1, x1+pwidth, y1);
                vec2d p3 = intersect(x+0.5, y+0.5, x1+(pwidth/2), y1+(pheight/2), x1+pwidth, y1, x1+pwidth, y1+pheight);
                vec2d p4 = intersect(x+0.5, y+0.5, x1+(pwidth/2), y1+(pheight/2), x1, y1, x1, y1+pheight);
                vec2d p = (p1!=nullvec)?(p1):((p2!=nullvec)?(p2):(p3!=nullvec)?(p3):(p4));
                vec2d q1 = intersect(x+0.5, y+0.5, x1+(pwidth/2), y1+(pheight/2), x, y+1, x+1, y+1);
                vec2d q2 = intersect(x+0.5, y+0.5, x1+(pwidth/2), y1+(pheight/2), x, y, x+1, y);
                vec2d q3 = intersect(x+0.5, y+0.5, x1+(pwidth/2), y1+(pheight/2), x+1, y, x+1, y+1);
                vec2d q4 = intersect(x+0.5, y+0.5, x1+(pwidth/2), y1+(pheight/2), x, y, x, y+1);
                vec2d q = (q1!=nullvec)?(q1):((q2!=nullvec)?(q2):(q3!=nullvec)?(q3):(q4));
                if (p != nullvec && q != nullvec) {
                  double dst = dist(p.x, p.y, q.x, q.y);
                  if (ang == PI*0.5 || ang == PI*1.5) {
                    entities[currplayer]->vel.x = 0;
                  } else {
                    entities[currplayer]->vel.y = 0;
                  }
                  entities[currplayer]->pos.x += cos(ang)*(dst);
                  entities[currplayer]->pos.y += sin(ang)*(dst);
                }
              }
            }
          }
        }
      }
      if (DEBUG_MODE) {
        cout << "Tick(5 of 4, I know, it's weird)!" << endl;
      }
      if (playmode == "console") {
        // Do something
        DrawDecal({0, 340}, tmp, {640, 20}, olc::Pixel(0, 0, 0, 127));
      }
      if (w.blocks[(int)(entities[currplayer]->pos.x)][(int)(entities[currplayer]->pos.y)].waterlevel > 3) {
        entities[currplayer]->applyforce({0, 0.01});
      } else {
        entities[currplayer]->applyforce({0, 0.05});
      }
      bool moving = false;
      if (playmode == "moving" && sysmode != "server") {
        if (GetKey(olc::Key::D).bHeld) {
          entities[currplayer]->applyforce({0.1, 0});
          moving = true;
        }
        if (GetKey(olc::Key::A).bHeld) {
          entities[currplayer]->applyforce({-0.1, 0});
          moving = true;
        }
        if (GetKey(olc::Key::SPACE).bPressed) {
          entities[currplayer]->applyforce({0, -2});
        }
        if (GetKey(olc::Key::K1).bHeld) {
          currinvslot = 0;
        }
        if (GetKey(olc::Key::K2).bHeld) {
          currinvslot = 1;
        }
        if (GetKey(olc::Key::K3).bHeld) {
          currinvslot = 2;
        }
        if (GetKey(olc::Key::K4).bHeld) {
          currinvslot = 3;
        }
        if (GetKey(olc::Key::K5).bHeld) {
          currinvslot = 4;
        }
        if (GetKey(olc::Key::K6).bHeld) {
          currinvslot = 5;
        }
        if (GetKey(olc::Key::K7).bHeld) {
          currinvslot = 6;
        }
        if (GetKey(olc::Key::K8).bHeld) {
          currinvslot = 7;
        }
        if (GetKey(olc::Key::K9).bHeld) {
          currinvslot = 8;
        }
        if (GetKey(olc::Key::K0).bHeld) {
          currinvslot = 9;
        }
        if (GetMouseWheel() < 0) {
          currinvbar = (currinvbar==3)?(0):(currinvbar+1);
          scrolltransparency = 127;
        }
        if (GetMouseWheel() > 0) {
          currinvbar = (currinvbar==0)?(3):(currinvbar-1);
          scrolltransparency = 127;
        }
      }
      if (GetKey(olc::Key::T).bPressed) {
        playmode = (playmode=="console")?"moving":"console";
      }
      if (DEBUG_MODE) {
        cout << "Tick(6 of 4, this is dumb...)!" << endl;
      }
      bool sendpack = false;
      if (w.blocks[(int)(entities[currplayer]->pos.x)][(int)(entities[currplayer]->pos.y)].waterlevel > 3) {
        sendpack = entities[currplayer]->update(0.3, 0.4, moving);
      } else {
        sendpack = entities[currplayer]->update(0.15, 0.25, moving);
      }
      if (sendpack && sysmode == "client") {
        // Send player update packet here
        int sz = 1/*Packet info*/ + sizeof(double)/*X coord*/ + sizeof(double)/*Y coord*/;
        unsigned char *packetdata = (unsigned char *)malloc(sz);
        unsigned char *ptx = (unsigned char *)malloc(sizeof(double));
        *(double *)(ptx) = entities[currplayer]->pos.x;
        unsigned char *pty = (unsigned char *)malloc(sizeof(double));
        *(double *)(pty) = entities[currplayer]->pos.y;
        packetdata[0] = (P_update_player* 16);
        for (int i = 0; i < sizeof(double); i++) {
          packetdata[1+i] = ptx[i];
          packetdata[1+sizeof(double)+i] = pty[i];
        }
        ENetPacket *packet = enet_packet_create((void *)packetdata, sz, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, 0, packet);
        enet_host_flush(host);
        free(packetdata);
      }
      invscroll = (invscroll*0.933) + (invscrollfinal*0.067);
      int i2 = 0;
      if (DEBUG_MODE) {
        cout << "Tick(7 of 4, ...)!" << endl;
      }
      if (inventory[currinvslot][currinvbar].name != "air") {
        if (inventory[currinvslot][currinvbar].amt > 1) {
          DrawString({10, 95}, string("slot: ") + inventory[currinvslot][currinvbar].name + string(" x") + to_string(inventory[currinvslot][currinvbar].amt), olc::Pixel(255, 255, 255, 127), 1);
        } else {
          DrawString({10, 95}, string("slot: ") + inventory[currinvslot][currinvbar].name, olc::Pixel(255, 255, 255, 127), 1);
        }
      }
      DrawString({10, 45}, string("FPS: ") + to_string(1.0/fElapsedTime), olc::Pixel(255, 255, 255, 127), 1);
      DrawString({10, 55}, string("XY: ") + to_string(entities[currplayer]->pos.x) + string(", ") + to_string(entities[currplayer]->pos.y), olc::Pixel(255, 255, 255, 127), 1);
      DrawString({10, 65}, string("vel-XY: ") + to_string(entities[currplayer]->vel.x) + string(", ") + to_string(entities[currplayer]->vel.y), olc::Pixel(255, 255, 255, 127), 1);
      DrawString({10, 75}, string("mouse-XY: ") + to_string(mx+iposx) + string(", ") + to_string(my+iposy), olc::Pixel(255, 255, 255, 127), 1);
      DrawDecal({8, 8}, invslot);
      invscrollfinal = currinvbar*24;
      if (abs(invscrollfinal - invscroll) < 1) {
        scrolltransparency *= 0.95;
      }
      for (int i = 0; i < 10; i++) {
        if (currinvbar == 0 && i == currinvslot) {
          DrawDecal({32 + (24*i), 8 + (24*0) - invscroll}, selslot, {1.0, 1.0}, olc::Pixel(255, 63, 63));
        } else {
          DrawDecal({32 + (24*i), 8 + (24*0) - invscroll}, invslot, {1.0, 1.0}, olc::Pixel(255, 63, 63, (currinvbar == 0) ? 255 : scrolltransparency));
        }
        if (currinvbar == 1 && i == currinvslot) {
          DrawDecal({32 + (24*i), 8 + (24*1) - invscroll}, selslot, {1.0, 1.0}, olc::Pixel(255, 255, 63));
        } else {
          DrawDecal({32 + (24*i), 8 + (24*1) - invscroll}, invslot, {1.0, 1.0}, olc::Pixel(255, 255, 63, (currinvbar == 1) ? 255 : scrolltransparency));
        }
        if (currinvbar == 2 && i == currinvslot) {
          DrawDecal({32 + (24*i), 8 + (24*2) - invscroll}, selslot, {1.0, 1.0}, olc::Pixel(63, 255, 63));
        } else {
          DrawDecal({32 + (24*i), 8 + (24*2) - invscroll}, invslot, {1.0, 1.0}, olc::Pixel(63, 255, 63, (currinvbar == 2) ? 255 : scrolltransparency));
        }
        if (currinvbar == 3 && i == currinvslot) {
          DrawDecal({32 + (24*i), 8 + (24*3) - invscroll}, selslot, {1.0, 1.0}, olc::Pixel(63, 127, 255));
        } else {
          DrawDecal({32 + (24*i), 8 + (24*3) - invscroll}, invslot, {1.0, 1.0}, olc::Pixel(63, 127, 255, (currinvbar == 3) ? 255 : scrolltransparency));
        }
        olc::vf2d relpos;
        if (inventory[i][0].name == "air") {
          relpos = {0, 0};
        } else {
          relpos = {(4*8) + (blockids[blockmap[inventory[i][currinvbar].name]].tx * 48.0), (1*8) + (blockids[blockmap[inventory[i][currinvbar].name]].ty * 24.0)};
        }
        olc::vf2d rlsize = {8.0, 8.0};
        if (inventory[i][currinvbar].name != "air") {
          DrawPartialDecal({34 + (24*i), 10}, texture, relpos, rlsize, {2.0, 2.0});
          if (inventory[i][currinvbar].amt > 1) {
            //SetDrawTarget(guilayer);
            DrawString({36 + (24*i), 32}, to_string(inventory[i][currinvbar].amt), olc::Pixel(255, 255, 255), 1);
            //SetDrawTarget(backgroundlayer);
          }
        }
      }
      if (DEBUG_MODE) {
        cout << "Tick(ended)!" << endl;
      }
      daytime += fElapsedTime / 60;
      if (daytime >= 24) daytime -= 24;
      selectdaycolors();
    }
    if (fElapsedTime < 2) {
      loadticks += fElapsedTime;
    }
    return true;
  }
};

int main() {
  if (enet_initialize () != 0) {
    cout << "<ENET_ERROR> ENet initialization error." << endl;
    return EXIT_FAILURE;
  }
  while (sysmode != "offline" && sysmode != "server" && sysmode != "client") {
    cout << "Select mode(server, client, offline): ";
    cin >> sysmode;
  }
  atexit(enet_deinitialize);
  if (sysmode == "server") {
    cout << "World name(world will be created if unexistent): ";
    cin >> worldname;
    address.host = ENET_HOST_ANY;
    address.port = 14142;
    host = enet_host_create(&address, 20, 1, 0, 0);
    if (host == NULL) {
      cout << "<ENET_ERROR> Error when making server host." << endl;
      exit(EXIT_FAILURE);
    }
  } else if (sysmode == "client") {
    string ipv4;
    cout << "Server IP: ";
    cin >> ipv4;
    cout << "Player name: ";
    cin >> currplayer;
    host = enet_host_create(NULL, 1, 1, 0, 0);
    if (host == NULL) {
      cout << "<ENET_ERROR> Error when making client host." << endl;
      exit(EXIT_FAILURE);
    }
    enet_address_set_host(&address, ipv4.c_str());
    address.port = 14142;
    peer = enet_host_connect(host, &address, 2, 0);    
    if (peer == NULL) {
       cout << "<ENET_ERROR> No available peers." << endl; 
       exit(EXIT_FAILURE);
    }
    if (enet_host_service(host, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
      cout << "<ENET_SUCCESS> Connection to " << ipv4 << ":14142 succeded." << endl;
    } else {
      enet_peer_reset(peer);
      cout << "<ENET_ERROR> Connection to " << ipv4 << ":14142' failed." << endl;
    }
  } else if (sysmode == "offline") {
    cout << "World name(world will be created if unexistent): ";
    cin >> worldname;
  }
  if (sysmode == "server" || sysmode == "offline") {
    for (int i = 0; i < 128; i++) {
      for (int j = 0; j < 8; j++) {
        chunkloaded[i][j] = true;
        chunkrequest[i][j] = false;
        chunkbroadcast[i][j] = (sysmode != "offline");
      }
    }
  } else if (sysmode == "client") {
    for (int i = 0; i < 128; i++) {
      for (int j = 0; j < 8; j++) {
        chunkloaded[i][j] = false;
        chunkrequest[i][j] = false;
        chunkbroadcast[i][j] = false;
      }
    }
    worldname = "server";
  }
  if (sysmode != "server") {
    initinv();
    JustSquares jsqr;
    if (jsqr.Construct(640, 360, 2, 2, false, false)) { // Temporally activated V-sync for the demo
      jsqr.Start();
    }
  } else {
    if (DEBUG_MODE) {
      cout << "Entering nogui server mode..." << endl;
    }
    nogui::init();
  }
  return 0;
}