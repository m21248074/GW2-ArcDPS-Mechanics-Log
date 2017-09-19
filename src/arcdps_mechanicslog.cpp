/*
* arcdps combat api example
*/

#include <stdint.h>
#include <stdio.h>
#include <Windows.h>
#include <vector>
#include <string>
#include "mechanic_ids.h"
#include "player.cpp"

/* arcdps export table */
typedef struct arcdps_exports {
	uintptr_t ext_size; /* arcdps internal use, ignore */
	uintptr_t ext_sig; /* pick a number between 0 and uint64_t max that isn't used by other modules */
	void* ext_wnd; /* wndproc callback, fn(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) */
	void* ext_combat; /* combat event callback, fn(cbtevent* ev, ag* src, ag* dst, char* skillname) */
	void* ext_imgui; /* id3dd9::present callback, before imgui::render, fn() */
	void* ext_options; /* id3dd9::present callback, appending to the end of options window in arcdps, fn() */
} arcdps_exports;

/* combat event */
typedef struct cbtevent {
	uint64_t time; /* timegettime() at time of event */
	uint64_t src_agent; /* unique identifier */
	uint64_t dst_agent; /* unique identifier */
	int32_t value; /* event-specific */
	int32_t buff_dmg; /* estimated buff damage. zero on application event */
	uint16_t overstack_value; /* estimated overwritten stack duration for buff application */
	uint16_t skillid; /* skill id */
	uint16_t src_instid; /* agent map instance id */
	uint16_t dst_instid; /* agent map instance id */
	uint16_t src_master_instid; /* master source agent map instance id if source is a minion/pet */
	uint8_t iss_offset; /* internal tracking. garbage */
	uint8_t iss_offset_target; /* internal tracking. garbage */
	uint8_t iss_bd_offset; /* internal tracking. garbage */
	uint8_t iss_bd_offset_target; /* internal tracking. garbage */
	uint8_t iss_alt_offset; /* internal tracking. garbage */
	uint8_t iss_alt_offset_target; /* internal tracking. garbage */
	uint8_t skar; /* internal tracking. garbage */
	uint8_t skar_alt; /* internal tracking. garbage */
	uint8_t skar_use_alt; /* internal tracking. garbage */
	uint8_t iff; /* from iff enum */
	uint8_t buff; /* buff application, removal, or damage event */
	uint8_t result; /* from cbtresult enum */
	uint8_t is_activation; /* from cbtactivation enum */
	uint8_t is_buffremove; /* buff removed. src=relevant, dst=caused it (for strips/cleanses). from cbtr enum  */
	uint8_t is_ninety; /* source agent health was over 90% */
	uint8_t is_fifty; /* target agent health was under 50% */
	uint8_t is_moving; /* source agent was moving */
	uint8_t is_statechange; /* from cbtstatechange enum */
	uint8_t is_flanking; /* target agent was not facing source */
	uint8_t is_shields; /* all or partial damage was vs barrier/shield */
	uint8_t result_local; /* internal tracking. garbage */
	uint8_t ident_local; /* internal tracking. garbage */
} cbtevent;

/* agent short */
typedef struct ag {
	char* name; /* agent name. may be null. valid only at time of event. utf8 */
	uintptr_t id; /* agent unique identifier */
	uint32_t prof; /* profession at time of event. refer to evtc notes for identification */
	uint32_t elite; /* elite spec at time of event. refer to evtc notes for identification */
	uint32_t self; /* 1 if self, 0 if not */
} ag;

/* proto/globals */
uint32_t cbtcount = 0;
arcdps_exports arc_exports;
char* arcvers;
void dll_init(HANDLE hModule);
void dll_exit();
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversionstr);
extern "C" __declspec(dllexport) void* get_release_addr();
arcdps_exports* mod_init();
uintptr_t mod_release();
uintptr_t mod_wnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
uintptr_t mod_combat(cbtevent* ev, ag* src, ag* dst, char* skillname);
uintptr_t mod_imgui();
uintptr_t mod_options();

Player players[10] = {Player()};
Player* get_player(uint16_t new_id);
void reset_all_player_stats();
Player* current_player = nullptr;
const unsigned int ms_per_tick = 40;// 1000/25
uint64_t start_time = 0;

struct mechanic
{
    std::string name = ""; //name of mechanic
    std::vector<uint16_t> ids; //skill ids;
    uint64_t frequency=2000; //minimum time between instances of this mechanic(ms)
    bool is_interupt=false;
    bool target_is_dst = true;
    bool fail_if_hit = true;

    bool is_valid_hit(cbtevent* &ev, ag* &src, ag* &dst)
    {
        uint16_t index = 0;
        bool correct_id = false;

        for(index=0;index<ids.size();index++)
        {
            if(ev->skillid==this->ids[index])
            {
                correct_id = true;
            }
        }

        if(correct_id)//correct skill id
        {
            if(target_is_dst)
            {
                current_player=get_player(ev->dst_instid);
            }
            else
            {
                current_player=get_player(ev->src_instid);
            }

            if(current_player
               && ((dst->prof < 10 && target_is_dst)
                    || (src -> prof < 10 && !target_is_dst))
               && ev->time > (current_player->last_hit_time+this->frequency)
               && (!is_interupt || current_player->last_stab_time < ev->time))
            {
                current_player->last_hit_time=ev->time;
                if(fail_if_hit)
                {
                    current_player->mechanics_failed++;
                }
                current_player->last_machanic=ev->skillid;
                return true;
            }
        }
        return false;
    }
};

struct vg_teleport : mechanic
{
    vg_teleport()
    {
        name="teleport"; //name of mechanic
        ids.push_back(MECHANIC_VG_TELEPORT_RAINBOW);
        ids.push_back(MECHANIC_VG_TELEPORT_GREEN);
    }
} vg_teleport;

struct vg_green : mechanic
{
    vg_green()
    {
        name="green"; //name of mechanic
        ids.push_back(MECHANIC_VG_GREEN_A);
        ids.push_back(MECHANIC_VG_GREEN_B);
        ids.push_back(MECHANIC_VG_GREEN_C);
        ids.push_back(MECHANIC_VG_GREEN_D);
        fail_if_hit = false;
    }
} vg_green;

struct gors_slam : mechanic
{
    gors_slam()
    {
        name="slam"; //name of mechanic
        ids.push_back(MECHANIC_GORS_SLAM); //skill id;
        is_interupt=true;
    }
} gors_slam;

struct gors_egg : mechanic
{
    gors_egg()
    {
        name="egg"; //name of mechanic
        ids.push_back(MECHANIC_GORS_EGG); //skill id;
    }

} gors_egg;

struct sloth_tantrum : mechanic
{
    sloth_tantrum()
    {
        name="tantrum"; //name of mechanic
        ids.push_back(MECHANIC_SLOTH_TANTRUM); //skill id;
    }

} sloth_tantrum;

struct sloth_bomb : mechanic
{
    sloth_bomb()
    {
        name="bomb"; //name of mechanic
        ids.push_back(MECHANIC_SLOTH_BOMB); //skill id;
        fail_if_hit = false;
    }

} sloth_bomb;

struct sloth_bomb_aoe : mechanic
{
    sloth_bomb_aoe()
    {
        name="bomb aoe"; //name of mechanic
        ids.push_back(MECHANIC_SLOTH_BOMB_AOE); //skill id;
    }

} sloth_bomb_aoe;

struct sloth_flame : mechanic
{
    sloth_flame()
    {
        name="flame breath"; //name of mechanic
        ids.push_back(MECHANIC_SLOTH_FLAME_BREATH); //skill id;
    }

} sloth_flame;

struct sloth_shake : mechanic
{
    sloth_shake()
    {
        name="shake"; //name of mechanic
        ids.push_back(MECHANIC_SLOTH_SHAKE); //skill id;
    }

} sloth_shake;

struct matt_hadouken : mechanic
{
    matt_hadouken()
    {
        name="hadouken"; //name of mechanic
        ids.push_back(MECHANIC_MATT_HADOUKEN_HUMAN); //skill id;
        ids.push_back(MECHANIC_MATT_HADOUKEN_ABOM); //skill id;
    }

} matt_hadouken;

struct matt_shard_reflect : mechanic
{
    matt_shard_reflect()
    {
        name="shard reflect"; //name of mechanic
        ids.push_back(MECHANIC_MATT_SHARD_HUMAN); //skill id;
        ids.push_back(MECHANIC_MATT_SHARD_ABOM); //skill id;
        target_is_dst = false;
    }

} matt_shard_reflect;

struct xera_half : mechanic
{
    xera_half()
    {
        name="half"; //name of mechanic
        ids.push_back(MECHANIC_XERA_HALF); //skill id;
    }

} xera_half;

struct xera_magic : mechanic
{
    xera_magic()
    {
        name="magic"; //name of mechanic
        ids.push_back(MECHANIC_XERA_MAGIC); //skill id;
        frequency=5000; //the bubbles don't happen very often
        fail_if_hit = false;
    }

} xera_magic;

struct xera_orb : mechanic
{
    xera_orb()
    {
        name="orb"; //name of mechanic
        ids.push_back(MECHANIC_XERA_ORB); //skill id;
    }

} xera_orb;

struct xera_orb_aoe : mechanic
{
    xera_orb_aoe()
    {
        name="orb aoe"; //name of mechanic
        ids.push_back(MECHANIC_XERA_ORB_AOE); //skill id;
    }

} xera_orb_aoe;

struct carin_teleport : mechanic
{
    carin_teleport()
    {
        name="teleport"; //name of mechanic
        ids.push_back(MECHANIC_CARIN_TELEPORT); //skill id;
    }

} carin_teleport;

struct carin_shard_reflect : mechanic
{
    carin_shard_reflect()
    {
        name="shard reflect"; //name of mechanic
        ids.push_back(MECHANIC_CARIN_SHARD); //skill id;
        target_is_dst = false;
    }

} carin_shard_reflect;

struct sam_shockwave : mechanic
{
    sam_shockwave()
    {
        name="shockwave"; //name of mechanic
        ids.push_back(MECHANIC_SAM_SHOCKWAVE); //skill id;
        is_interupt=true;
    }

} sam_shockwave;

struct sam_slap : mechanic
{
    sam_slap()
    {
        name="slap"; //name of mechanic
        ids.push_back(MECHANIC_SAM_SLAP); //skill id;
        is_interupt=true;
    }

} sam_slap;

struct deimos_oil : mechanic
{
    deimos_oil()
    {
        name="oil"; //name of mechanic
        ids.push_back(MECHANIC_DEIMOS_OIL); //skill id;
    }

} deimos_oil;

struct deimos_smash : mechanic
{
    deimos_smash()
    {
        name="smash"; //name of mechanic
        ids.push_back(MECHANIC_DEIMOS_SMASH); //skill id;
        ids.push_back(MECHANIC_DEIMOS_SMASH_INITIAL);
        is_interupt=true;
    }

} deimos_smash;

Player* get_player(uint16_t new_id)
{
    for(unsigned int index=0;index<sizeof(players);index++)
    {
        if(players[index].id==0)
        {
            players[index] = Player(new_id);
            return &players[index];
        }
        else if(players[index].id == new_id)
        {
            return &players[index];
        }
    }
    return nullptr;
}

void reset_all_player_stats()
{
    for(unsigned int index=0;index<10;index++)
    {
        players[index].reset_all();
    }
}

inline int get_elapsed_time(uint64_t &current_time)
{
    return (current_time-start_time)/1000;
}

/* dll main -- winapi */
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ulReasonForCall, LPVOID lpReserved) {
	switch (ulReasonForCall) {
		case DLL_PROCESS_ATTACH: dll_init(hModule); break;
		case DLL_PROCESS_DETACH: dll_exit(); break;

		case DLL_THREAD_ATTACH:  break;
		case DLL_THREAD_DETACH:  break;
	}
	return 1;
}

/* dll attach -- from winapi */
void dll_init(HANDLE hModule) {
	return;
}

/* dll detach -- from winapi */
void dll_exit() {
	return;
}

/* export -- arcdps looks for this exported function and calls the address it returns */
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversionstr) {
	arcvers = arcversionstr;
	return mod_init;
}

/* export -- arcdps looks for this exported function and calls the address it returns */
extern "C" __declspec(dllexport) void* get_release_addr() {
	arcvers = 0;
	return mod_release;
}

/* initialize mod -- return table that arcdps will use for callbacks */
arcdps_exports* mod_init() {

	/* demo */
	AllocConsole();

	/* big buffer */
	char buff[4096];
	char* p = &buff[0];
	p += _snprintf(p, 400, "==== mechanics log ====\n");

	/* print */
	DWORD written = 0;
	HANDLE hnd = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleA(hnd, &buff[0], p - &buff[0], &written, 0);

	/* for arcdps */
	arc_exports.ext_size = sizeof(arcdps_exports);
	arc_exports.ext_sig = 0x81004122;//from random.org
	arc_exports.ext_wnd = mod_wnd;
	arc_exports.ext_combat = mod_combat;
	arc_exports.ext_imgui = mod_imgui;
	arc_exports.ext_options = mod_options;
	return &arc_exports;
}

/* release mod -- return ignored */
uintptr_t mod_release() {
	FreeConsole();
	return 0;
}

/* window callback -- return is assigned to umsg (return zero to not be processed by arcdps or game) */
uintptr_t mod_wnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
#if 0
	/* big buffer */
	char buff[4096];
	char* p = &buff[0];

	/* common */
	p += _snprintf(p, 400, "==== wndproc %llx ====\n", hWnd);
	p += _snprintf(p, 400, "umsg %u, wparam %lld, lparam %lld\n", uMsg, wParam, lParam);

	/* print */
	DWORD written = 0;
	HANDLE hnd = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleA(hnd, &buff[0], p - &buff[0], &written, 0);
#endif
	return uMsg;
}

/* combat callback -- may be called asynchronously. return ignored */
/* one participant will be party/squad, or minion of. no spawn statechange events. despawn statechange only on marked boss npcs */
uintptr_t mod_combat(cbtevent* ev, ag* src, ag* dst, char* skillname) {

	/* big buffer */
	char buff[4096];
	char* p = &buff[0];

	/* ev is null. dst will only be valid on tracking add. skillname will also be null */
	if (!ev)
    {

	}

	/* combat event. skillname may be null. non-null skillname will remain static until module is unloaded. refer to evtc notes for complete detail */
	else
    {

		/* common */

		/* statechange */
		if (ev->is_statechange)
        {
            if(ev->is_statechange==1
                && src->self)
            {
                    start_time = ev->time;
                    reset_all_player_stats();
            }
		}

		/* activation */
		else if (ev->is_activation)
        {

		}

		/* buff remove */
		else if (ev->is_buffremove)
        {
            if (ev->skillid==1122)//if it's stability
            {
                current_player=get_player(ev->dst_instid);
                if(current_player)
                {
                    current_player->last_stab_time = ev->time+ms_per_tick;//cut the ending time of stab early
                }
            }
		}

		/* buff */
		else if (ev->buff)
        {
            if (ev->skillid==1122)//if it's stability
            {
                current_player=get_player(ev->dst_instid);
                if(current_player
                   && current_player->last_stab_time < (ev->time+ev->value))//if the new stab will last longer than any possible old stab
                {
                    current_player->last_stab_time = ev->time+ev->value+ms_per_tick;//add prediction of when new stab will end
                }
            }
		}

        if(ev->dst_agent)
        {
            //if attack hits (not block/evaded/invuln/miss)
            //and it's a player, not a summon
            if(ev->result==0 || ev->result==1 || ev->result==2 || ev->result==5 || ev->result==8)
            {

                //vg teleport
                if(vg_teleport.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s was teleported\n",get_elapsed_time(ev->time), dst->name);
                }
#if 0
                //vg green circle
                if(vg_green.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s stood in the green circle\n",get_elapsed_time(ev->time), dst->name);
                }
#endif
                //gors slam
                if(gors_slam.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s was slammed\n",get_elapsed_time(ev->time), dst->name);
                }

                //gors egg
                if(gors_egg.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s was egged\n",get_elapsed_time(ev->time), dst->name);
                }

                //sloth tantrum
                if(sloth_tantrum.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s was hit with tantrum\n",get_elapsed_time(ev->time), dst->name);
                }

                //sloth bomb
                if(sloth_bomb.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s got a bomb\n",get_elapsed_time(ev->time), dst->name);
                }

                //sloth bomb aoe
                if(sloth_bomb_aoe.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s stood in a bomb aoe\n",get_elapsed_time(ev->time), dst->name);
                }

                //sloth flame breath
                if(sloth_flame.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s was hit by flame breath\n",get_elapsed_time(ev->time), dst->name);
                }

                //sloth shake
                if(sloth_shake.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s was hit by shake\n",get_elapsed_time(ev->time), dst->name);
                }

                //matti hadouken
                if(matt_hadouken.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s was hadoukened\n",get_elapsed_time(ev->time), dst->name);
                }

                 //matti shard reflect
                if(matt_shard_reflect.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s reflected shards\n",get_elapsed_time(ev->time), src->name);
                }

                //xera half
                if(xera_half.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s stood in the red half\n",get_elapsed_time(ev->time), dst->name);
                }

                //xera magic
                if(xera_magic.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s has magic\n",get_elapsed_time(ev->time), dst->name);
                }

                //xera orb
                if(xera_orb.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s touched an orb\n",get_elapsed_time(ev->time), dst->name);
                }

                //xera orb aoe
                if(xera_orb_aoe.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s stood in an orb aoe\n",get_elapsed_time(ev->time), dst->name);
                }

                //carin teleport
                if(carin_teleport.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s was teleported\n",get_elapsed_time(ev->time), dst->name);
                }

                //carin shard reflect
                if(carin_shard_reflect.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s reflected shards\n",get_elapsed_time(ev->time), dst->name);
                }

                //sam shockwave
                if(sam_shockwave.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s was hit by shockwave\n",get_elapsed_time(ev->time), dst->name);
                }

                //sam slap
                if(sam_slap.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s was slapped\n",get_elapsed_time(ev->time), dst->name);
                }

                //deimos oil
                if(deimos_oil.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s touched an oil\n",get_elapsed_time(ev->time), dst->name);
                }

                //deimos smash
                if(deimos_smash.is_valid_hit(ev, src, dst))
                {
                    p +=  _snprintf(p, 400, "%d: %s was hit by smash\n",get_elapsed_time(ev->time), dst->name);
                }
            }
        }

		/* common */
		cbtcount += 1;
	}

	/* print */
	DWORD written = 0;
	HANDLE hnd = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleA(hnd, &buff[0], p - &buff[0], &written, 0);
	return 0;
}

uintptr_t mod_imgui()
{
    return 0;
}

uintptr_t mod_options()
{
    return 0;
}
