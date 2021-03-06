#include "p2Defs.h"
#include "p2Log.h"
#include "j1App.h"
#include "j1Input.h"
#include "j1Textures.h"
#include "j1Audio.h"
#include "j1Render.h"
#include "j1Window.h"
#include "j1Map.h"
#include "j1Collisions.h"
#include "j1EntityManager.h"
#include "j1Player.h"
#include "j1Scene.h"


j1Player::j1Player() : j1Entity()
{
	rect = { 10, 10, 10, 10 };
}

// Destructor ---------------------------------
j1Player::~j1Player()
{}

// Called before the first frame
bool j1Player::Start()
{
	player_position.x = App->map->current_spawn_point.x;
	player_position.y = App->map->current_spawn_point.y;
	player_texture = App->tex->Load(PATH(player_folder.GetString(), "axolotl.png"));
	die_texture = App->tex->Load(PATH(player_folder.GetString(), "death.png"));
	
	idle_animation.name = "idle";
	idle_animation.max_frame = 5;

	for (int i = 0; i < 5; i++)
	{
		idle_animation.animation_rect[i] = { 36 * i, 0, 36, 38};
	}

	jump_animation.name = "jump";
	jump_animation.max_frame = 4;
	jump_animation.loop = false;

	for (int i = 0; i < 4; i++)
	{
		jump_animation.animation_rect[i] = { 36 * i, 38, 36, 38};
	}

	run_animation.name = "run";
	run_animation.max_frame = 6;

	for (int i = 0; i < 6; i++)
	{
		run_animation.animation_rect[i] = { 36 * i, 38 * 2, 36, 38};
	}

	die_animation.name = "die";
	die_animation.max_frame = 7;
	die_animation.loop = false;
	die_animation.max_frame_time = 0.07f;

	for (int i = 0; i < 7; i++)
	{
		die_animation.animation_rect[i] = { 40 * i, 0, 40, 44};
	}

	current_animation = &idle_animation;

	return true;
}

// Called before render is available ----------
bool j1Player::Awake(pugi::xml_node& module_node)
{
	LOG("Init player");
	bool ret = true;

	player_folder.create(module_node.child("folder").child_value());

	player_speed.x = module_node.child("speed").attribute("x").as_uint();
	player_speed.y = module_node.child("speed").attribute("y").as_uint();

	gravity = module_node.child("attributes").attribute("gravity").as_float();
	god_mode = module_node.child("attributes").attribute("god_mode").as_bool();

	rect.x = module_node.child("rect").attribute("x").as_uint();
	rect.y = module_node.child("rect").attribute("y").as_uint();
	rect.w = module_node.child("rect").attribute("w").as_uint();
	rect.h = module_node.child("rect").attribute("h").as_uint();

	return ret;
}

bool j1Player::Draw(float dt)
{
	SDL_Texture* texture = player_texture;
	if (dead)
	{
		texture = die_texture;
	}

	App->render->Blit(texture, player_position.x, player_position.y, &current_animation->GetCurrentFrame(), flip);

	if (App->map->debug_mode_active)
	{
		App->render->DrawQuad(rect, 20, 220, 20, 255, false, true);
	}

	frame_time += dt;

	if (frame_time > current_animation->max_frame_time)
	{
		frame_time = 0.0f;
		current_animation->GetNextFrame();
	}

	return true;
}

void j1Player::GodMode(float dt)
{
	if (App->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT)
	{
		player_speed.y = 130.0f * dt;

		rect.y -= player_speed.y;

		SDL_Rect* collider = CheckCollisions(TOP);

		if (collider == nullptr)
		{
			player_position.y = rect.y;
		}
		else
		{
			rect.y = collider->y + collider->h;
			player_position.y = rect.y;
		}
	}

	if (App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT)
	{
		player_speed.y = 130.0f * dt;

		rect.y += player_speed.y;

		SDL_Rect* collider = CheckCollisions(BOTTOM);

		if (collider == nullptr)
		{
			player_position.y = rect.y;
		}
		else
		{
			rect.y = collider->y - rect.h;
			player_position.y = rect.y;
		}
	}

}

void j1Player::ChooseAnimation(float dt)
{
	if (player_speed.y * dt > 1.0f || player_speed.y * dt < -1.0f)
	{
		current_animation = &jump_animation;
		return;
	}

	if (player_speed.x * dt > 0.001f || player_speed.x * dt < -0.001f)
	{
		current_animation = &run_animation;
		return;
	}

	current_animation = &idle_animation;
}

void j1Player::ApplyGravity(float dt)
{
	float g = player_speed.y * dt;

	player_position.y += g;
	rect.y = (int)player_position.y;
	SDL_Rect* collider = CheckCollisions(BOTTOM);

	if (collider != nullptr)
	{
		rect.y = collider->y - rect.h;
		player_position.y = rect.y;
		player_speed.y = 0.0f;
	}
}

void j1Player::Jump(float dt)
{
	player_position.y += player_speed.y * dt;
	rect.y = (int)player_position.y;
	SDL_Rect* collider = CheckCollisions(TOP);
		
	if (collider != nullptr)
	{
		rect.y = collider->y + collider->h;
		player_position.y = rect.y;
	}
}

bool j1Player::HasPlayerDied()
{
	bool ret = false;
	p2List_item<SDL_Rect>* item = App->collisions->death_triggers.start;

	for (item; item != App->collisions->death_triggers.end; item = item->next)
	{
		ret = App->collisions->CheckCollision(rect, item->data);

		if (ret)
			return ret;
	}

	return ret;
}

bool j1Player::IsCollidingWithEnemy()
{
	bool ret = false;
	p2List_item<j1Entity*>* item = App->entity_manager->entities.start;

	for (item; item != NULL; item = item->next)
	{
		if (item->data == this)
		{
			continue;
		}

		ret = App->collisions->CheckCollision(rect, item->data->rect);
		
		if (ret)
		{
  			if (player_speed.y > 0.01f)
			{
				App->entity_manager->DestroyEntity(item->data);
				return false;
			}

			return ret;
		}
	}

	return ret;
}

bool j1Player::HasPlayerWon()
{
	bool ret = false;
	p2List_item<SDL_Rect>* item = App->collisions->win_triggers.start;

	for (item; item != App->collisions->win_triggers.end; item = item->next)
	{
		ret = App->collisions->CheckCollision(rect, item->data);

		if (ret)
			return ret;
	}

	return ret;
}

SDL_Rect* j1Player::CheckCollisions(Direction direction)
{
	bool ret = false;
	p2List_item<SDL_Rect>* item = App->collisions->no_walkable_tiles.start;

	for (item; item != App->collisions->no_walkable_tiles.end; item = item->next)
	{
		ret = App->collisions->CheckCollision(rect, item->data);
		
		if (!ret)
			continue;
		
		switch (direction)
		{
		case BOTTOM:
			if (rect.y + rect.h > item->data.y)
				return &item->data;
			break;
		
		case TOP:
			if (rect.y < item->data.y + item->data.h)
				return &item->data;
			break;

		case RIGHT:
			if (rect.x + rect.w > item->data.x)
				return &item->data;
			break;

		case LEFT:
			if (rect.x < item->data.x + item->data.w)
				return &item->data;
			break;
		}
	}

	return nullptr;
}

// Update ------------------------------------
bool j1Player::Update(float dt)
{
	bool ret = true;

	if (dead)
	{
		if (current_animation->current_frame == 6)
		{
			player_position.x = App->map->current_spawn_point.x;
			player_position.y = App->map->current_spawn_point.y;
			player_speed.SetToZero();
			App->entity_manager->ResetEntities();
			dead = false;
		}

		return ret;
	}
	
	rect.x = player_position.x;
	rect.y = player_position.y;
		
	if (App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
	{
		player_speed.x = 130.0f * dt;

		player_position.x -= player_speed.x;
		rect.x = (int)player_position.x;

		SDL_Rect* collider = CheckCollisions(LEFT);

		if (collider != nullptr)
		{
			rect.x = collider->x + collider->w;
			player_position.x = rect.x;
		}

		flip = false;
	}

	if (App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)
	{
		player_speed.x = 130.0f * dt;

		player_position.x += player_speed.x;
		rect.x = (int)player_position.x;

		SDL_Rect* collider = CheckCollisions(RIGHT);

		if (collider != nullptr)
		{
			rect.x = collider->x - rect.w;
			player_position.x = rect.x;
		}
		flip = true;
	}

	if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN)
	{
		rect.y += 1;
		SDL_Rect* collider = CheckCollisions(BOTTOM);
		rect.y = player_position.y;
		if(collider != nullptr)
		{
			jump_animation.current_frame = 0;
			player_speed.y = -300;
		}
	}

	player_speed.y += gravity * dt;

	if (player_speed.y < 0)
		Jump(dt);
	else
	{
		if (god_mode)
		{
			GodMode(dt);
			return ret;
		}
		ApplyGravity(dt);
	}
	ChooseAnimation(dt);

	if (HasPlayerWon() == true)
	{
		App->scene->HasWon = true;
		return true;
	}

	if (IsCollidingWithEnemy() || HasPlayerDied() == true)
	{
		dead = true;
		die_animation.current_frame = 0;
		current_animation = &die_animation;
	}

	player_speed.x = 0.0f;

	return ret;
}

// Called before quitting --------------------
bool j1Player::CleanUp()
{
	LOG("Destroying player");

	return true;
}

// Save & Load ------------------------------
bool j1Player::Save(pugi::xml_node& module_node) const
{
	pugi::xml_node child_player_position = module_node.append_child("position");

	child_player_position.append_attribute("x").set_value(player_position.x);
	child_player_position.append_attribute("y").set_value(player_position.y);

	pugi::xml_node child_player_god_mode = module_node.append_child("god_mode");

	child_player_god_mode.append_attribute("value").set_value(god_mode);

	return true;
}

bool j1Player::Load(pugi::xml_node& module_node)
{
	player_position.x = module_node.child("position").attribute("x").as_uint();
	player_position.y = module_node.child("position").attribute("y").as_uint();

	god_mode = module_node.child("god_mode").attribute("value").as_bool();

	player_speed.SetToZero();
	return true;
}