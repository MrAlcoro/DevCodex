#ifndef __j1PLAYER_H__
#define __j1PLAYER_H__

#include "j1Module.h"
#include "p2Point.h"
#include "p2List.h"
#include "SDL\include\SDL_rect.h"

struct SDL_Texture;
struct SDL_Rect;


enum CollisionDirection
{
	BOTTOM = 0,
	TOP,
	RIGHT,
	LEFT
};

class j1Player : public j1Module
{
public:

	j1Player();

	// Destructor
	virtual ~j1Player();

	// Called before the first frame
	bool Start();

	// Called before render is available
	bool Awake(pugi::xml_node& module_node);

	// Update
	bool Update(float dt);

	// Called before quitting
	bool CleanUp();

	// Save & Load
	bool Save(pugi::xml_node& module_node) const;
	bool Load(pugi::xml_node& module_node);

	void ApplyGravity(float dt);
	void Jump(float dt);
	SDL_Rect* CheckCollisions(CollisionDirection direction);
	float Lerp(float a, float b, float f);
	void Draw();

public:
	iPoint player_position;

	float gravity = 15.8f;
	fPoint player_speed;
	
private:
	p2SString player_folder;
	SDL_Texture* player_texture;
	SDL_Rect player_rect;
};
#endif //__j1PLAYER_H__