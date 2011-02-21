#include <engine/e_server_interface.h>
#include <engine/e_config.h>
#include <game/generated/g_protocol.hpp>
#include <game/server/gamecontext.hpp>
#include "projectile.hpp"


//////////////////////////////////////////////////
// projectile
//////////////////////////////////////////////////
PROJECTILE::PROJECTILE(int type, int owner, vec2 pos, vec2 dir, int span,
	int damage, int flags, float force, int sound_impact, int weapon, bool move_gun)
: ENTITY(NETOBJTYPE_PROJECTILE)
{
	this->type = type;
	this->pos = pos;
	this->direction = dir;
	this->lifespan = span;
	this->owner = owner;
	this->flags = flags;
	this->force = force;
	this->damage = damage;
	this->sound_impact = sound_impact;
	this->weapon = weapon;
	this->bounce = 0;
	this->start_tick = server_tick();
	this->move_gun = move_gun;
	bouncing = 2;
	game.world.insert_entity(this);
}

void PROJECTILE::reset()
{	
	if (lifespan>-2)
		game.world.destroy_entity(this);
}

vec2 PROJECTILE::get_pos(float time)
{
	float curvature = 0;
	float speed = 0;
	if(type == WEAPON_GRENADE)
	{
		curvature = tuning.grenade_curvature;
		speed = tuning.grenade_speed;
	}
	else if(type == WEAPON_SHOTGUN)
	{
		curvature = tuning.shotgun_curvature;
		speed = tuning.shotgun_speed;
	}
	else if(type == WEAPON_GUN)
	{
		curvature = tuning.gun_curvature;
		speed = tuning.gun_speed;
	}
	if(move_gun)//entity gun
	{
		curvature = 0;
		speed = 500;
	}
	
	return calc_pos(pos, direction, curvature, speed, time);
}


void PROJECTILE::tick()
{
	
	float pt = (server_tick()-start_tick-1)/(float)server_tickspeed();
    float dt = (server_tick()-start_tick-2)/(float)server_tickspeed();
	float ct = (server_tick()-start_tick)/(float)server_tickspeed();
	vec2 prevpos = get_pos(pt);
	vec2 curpos = get_pos(ct);
	vec2 dirpos = get_pos(dt);

	lifespan--;
	
	vec2 new_pos;

	int collide = col_intersect_line(prevpos, curpos, &curpos, &new_pos);

	if(!collide)
		collide = col_check_platte((int)curpos.x, (int)curpos.y);
	CHARACTER *ownerchar = game.get_player_char(owner);
	CHARACTER *targetchr = game.world.intersect_character(prevpos, curpos, 6.0f, curpos, ownerchar);

	/*if (collide && (flags & BOUNCE) && lifespan >= 0 && move_gun) 
	{
		vec2 temp_pos = dirpos;
        vec2 temp_dir = curpos-dirpos;
		move_point(&temp_pos, &temp_dir, 1.0f, 0);
		pos = temp_pos;
		direction = normalize(temp_dir)*distance(vec2(0,0), direction)*0.001f*750;
		start_tick = server_tick();
	}*/


	if (collide && move_gun)
	{
			start_tick=server_tick();
			pos=new_pos;
			if (bouncing==2)
				direction.x=-direction.x;
			else if (bouncing==1)
				direction.y=-direction.y;
			pos+=direction;
	}
	else if(targetchr && move_gun)
	{
		targetchr->core.jumped = 0;
		targetchr->input.jump = 1;
		targetchr->move_gun_jump = true;
	}
	else if(((targetchr && !game.controller->is_race()) || collide || lifespan < 0) && !move_gun)
	{
		if(lifespan >= 0 || weapon == WEAPON_GRENADE)
			game.create_sound(curpos, sound_impact);

		if(flags & PROJECTILE_FLAGS_EXPLODE)
			game.create_explosion(curpos, owner, weapon, false);
		else if((targetchr && !game.controller->is_race()))
			targetchr->take_damage(direction * max(0.001f, force), damage, owner, weapon);

		game.world.destroy_entity(this);
	}
	int z = col_is_teleport((int)curpos.x,curpos.y);
  	if(config.sv_teleport && z && config.sv_teleport_grenade && weapon == WEAPON_GRENADE && game.controller->is_race())
  	{
 		pos = teleport(z);
  		start_tick = server_tick();
	}
}

void PROJECTILE::fill_info(NETOBJ_PROJECTILE *proj)
{
	proj->x = (int)pos.x;
	proj->y = (int)pos.y;
	proj->vx = (int)(direction.x*100.0f);
	proj->vy = (int)(direction.y*100.0f);
	proj->start_tick = start_tick;
	proj->type = type;
}

void PROJECTILE::snap(int snapping_client)
{
	float ct = (server_tick()-start_tick)/(float)server_tickspeed();
	
	if(networkclipped(snapping_client, get_pos(ct)))
		return;

	NETOBJ_PROJECTILE *proj = (NETOBJ_PROJECTILE *)snap_new_item(NETOBJTYPE_PROJECTILE, id, sizeof(NETOBJ_PROJECTILE));
	fill_info(proj);
}
