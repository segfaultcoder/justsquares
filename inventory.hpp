#ifndef __INVENTORY
#define __INVENTORY

class inventoryslot {
  public:

  string name;
  int amt;

  inventoryslot(string _name, int _amt) {
    name = _name; amt = _amt;
  }

  inventoryslot() {
    name = "air";
    amt = 0;
  }
};

inventoryslot inventory[10][4]; // 40 slots

int currinvslot = 0;
int currinvbar = 0;

void initinv() {
  for (int i = 0; i < 10; i++) {
  	for (int j = 0; j < 4; j++) {
  		inventory[i][j] = inventoryslot("air", 0);
  	}
  }
}

bool pushtoinv(string block, int amt) {
	if (block != "air" && amt > 0) {
		for (int j = 0; j < 4; j++) {
			for (int i = 0; i < 10; i++) {
	  		if (inventory[i][j].name == block) {
	  			if (inventory[i][j].amt < 100) {
	  				inventory[i][j].amt++;
	  				return true;
	  			}
	  		}
	  	}
	  }
	  for (int j = 0; j < 4; j++) {
	  	for (int i = 0; i < 10; i++) {
	   		if (inventory[i][j].name == "air") {
	 				inventory[i][j].name = block;
	 				inventory[i][j].amt = 1;
	 				return true;
	   		}
	   	}
	  }
	}
	cout << "Block '" << block << "' deleted." << endl;
  return false;
}

bool takefrominv(string block, int amt, int priority) {
	if (priority >= 0 && priority < 10) {
		if (inventory[priority][currinvbar].name == block) {
			inventory[priority][currinvbar].amt--;
			if (inventory[priority][currinvbar].amt == 0) {
				inventory[priority][currinvbar].name = "air";
			}
			return true;
		}
	}
	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 10; i++) {
  		if (inventory[i][j].name == block) {
 				inventory[i][j].amt--;
 				if (inventory[i][j].amt == 0) {
 					inventory[i][j].name = "air";
 				}
 				return true;
  		}
  	}
  }
  return false;
}
#endif