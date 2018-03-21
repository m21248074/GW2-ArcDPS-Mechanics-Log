#pragma once

#include <string>
#include <vector>

#include "arcdps_datastructures.h"
#include "player.h"
#include "skill_ids.h"
#include "npc_ids.h"
#include "helpers.h"

extern bool have_added_line_break;
extern uint64_t last_mechanic_time;
extern uint64_t line_break_frequency;
extern bool has_logged_mechanic;

const unsigned int ms_per_tick = 40;// 1000/25
const unsigned int combatapi_delay = 5000;

struct mechanic
{
    std::string name; //name of mechanic
    std::vector<uint16_t> ids; //skill ids;
    uint32_t boss_id;//required boss id, ignored if 0
    uint64_t frequency_player; //minimum time between instances of this mechanic per player(ms)
    uint64_t frequency_global; //minimum time between instances of this mechanic globally(ms)
    uint64_t last_hit_time; //time of last instance of mechanic
    uint8_t is_buffremove;
	int32_t overstack_value;//required overstack value
    bool is_interupt;
    bool is_multihit;
    bool target_is_dst;
    bool fail_if_hit;
    bool valid_if_down; //mechanic counts if player is in down-state

	bool can_evade;
	bool can_block;
	bool can_invuln;

	uint16_t verbosity;

    mechanic();

    float is_valid_hit(cbtevent* ev, ag* src, ag* dst, game_state* gs);
    bool (*special_requirement)(const mechanic &current_mechanic, cbtevent* ev, ag* src, ag* dst, Player* current_player);
    float (*special_value)(const mechanic &current_mechanic, cbtevent* ev, ag* src, ag* dst, Player* current_player);

    mechanic set_name(std::string const new_name) {this->name = new_name; return *this;}
    mechanic set_ids(std::initializer_list<uint16_t> const new_ids) {this->ids = std::vector<uint16_t>(new_ids); return *this;}
    mechanic set_boss_id(uint32_t const new_boss_id) {this->boss_id = new_boss_id; return *this;}
    mechanic set_frequency_player(uint64_t const new_frequency_player) {this->frequency_player = new_frequency_player; return *this;}
    mechanic set_frequency_global(uint64_t const new_frequency_global) {this->frequency_global = new_frequency_global; return *this;}
    mechanic set_is_buffremove(uint8_t const new_is_buffremove) {this->is_buffremove = new_is_buffremove; return *this;}
	mechanic set_overstack_value(int32_t const new_overstack_value) { this->overstack_value = new_overstack_value; return *this; }
    mechanic set_is_interupt(bool const new_is_interupt) {this->is_interupt = new_is_interupt; return *this;}
    mechanic set_is_multihit(bool const new_is_multihit) {this->is_multihit = new_is_multihit; return *this;}
    mechanic set_target_is_dst(bool const new_target_is_dst) {this->target_is_dst = new_target_is_dst; return *this;}
    mechanic set_fail_if_hit(bool const new_fail_if_hit) {this->fail_if_hit = new_fail_if_hit; return *this;}
    mechanic set_valid_if_down(bool const new_valid_if_down) {this->valid_if_down = new_valid_if_down; return *this;}
	mechanic set_can_evade(bool const new_can_evade) { this->can_evade = new_can_evade; return *this; }
	mechanic set_can_block(bool const new_can_block) { this->can_block = new_can_block; return *this; }
	mechanic set_can_invuln(bool const new_can_invuln) { this->can_invuln = new_can_invuln; return *this; }
	mechanic set_verbosity(uint16_t const new_verbosity) { this->verbosity = new_verbosity; return *this; }

    mechanic set_special_requirement(bool (*new_special_requirement)(const mechanic &current_mechanic, cbtevent* ev, ag* src, ag* dst, Player* current_player)) {this->special_requirement = new_special_requirement; return *this;}
    mechanic set_special_value(float (*new_special_value)(const mechanic &current_mechanic, cbtevent* ev, ag* src, ag* dst, Player* current_player)) {this->special_value = new_special_value; return *this;}
};

bool default_requirement(const mechanic &current_mechanic, cbtevent* ev, ag* src, ag* dst, Player* current_player);
bool special_requirement_conjure(const mechanic &current_mechanic, cbtevent* ev, ag* src, ag* dst, Player* current_player);
bool special_requirement_dhuum_snatch(const mechanic &current_mechanic, cbtevent* ev, ag* src, ag* dst, Player* current_player);

float default_value(const mechanic &current_mechanic, cbtevent* ev, ag* src, ag* dst, Player* current_player);
float special_value_dhuum_shackles(const mechanic &current_mechanic, cbtevent* ev, ag* src, ag* dst, Player* current_player);

const uint16_t max_deimos_oils = 3;
struct deimos_oil
{
	uintptr_t id = 0;
	uint64_t last_touch_time = 0;
};

extern deimos_oil deimos_oils[max_deimos_oils];

bool special_requirement_deimos_oil(const mechanic & current_mechanic, cbtevent * ev, ag * src, ag * dst, Player * current_player);

bool special_requirement_on_self(const mechanic & current_mechanic, cbtevent * ev, ag * src, ag * dst, Player * current_player);


extern std::vector <mechanic> mechanics;
