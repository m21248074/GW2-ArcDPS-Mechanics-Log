#include "mechanics.h"

bool has_logged_mechanic = false;

Mechanic::Mechanic() noexcept
{
	special_requirement = requirementDefault;
	special_value = valueDefault;
}

Mechanic::Mechanic(std::string new_name, std::initializer_list<uint32_t> new_ids, Boss* new_boss, bool new_fail_if_hit, bool new_valid_if_down, int new_verbosity,
	bool new_is_interupt, bool new_is_multihit, int new_target_location,
	uint64_t new_frequency_player, uint64_t new_frequency_global, int32_t new_overstack_value, int32_t new_value,
	uint8_t new_is_activation, uint8_t new_is_buffremove,
	bool new_can_evade, bool new_can_block, bool new_can_invuln,
	bool(*new_special_requirement)(const Mechanic &current_mechanic, cbtevent* ev, ag* ag_src, ag* ag_dst, Player * player_src, Player * player_dst, Player* current_player),
	int64_t(*new_special_value)(const Mechanic &current_mechanic, cbtevent* ev, ag* ag_src, ag* ag_dst, Player * player_src, Player * player_dst, Player* current_player),
	std::string new_name_internal, std::string new_description)
{
	name = new_name;

	std::copy(new_ids.begin(), new_ids.end(), ids);
	ids_size = new_ids.size();

	boss = new_boss;
	fail_if_hit = new_fail_if_hit;
	valid_if_down = new_valid_if_down;
	verbosity = new_verbosity;

	is_interupt = new_is_interupt;
	is_multihit = new_is_multihit;
	target_is_dst = new_target_location;

	frequency_player = new_frequency_player;
	frequency_global = new_frequency_global;

	overstack_value = new_overstack_value;
	value = new_value;

	is_activation = new_is_activation;
	is_buffremove = new_is_buffremove;

	can_evade = new_can_evade;
	can_block = new_can_block;
	can_invuln = new_can_invuln;

	special_requirement = new_special_requirement;
	special_value = new_special_value;

	name_internal = new_name_internal;
	description = new_description;

	name_chart = (new_boss ? new_boss->name : "")
		+ " - " + new_name;
	name_ini = getIniName();//TODO: replace this with the code from getIniName() once all mechanic definitions use new style
}

int64_t Mechanic::isValidHit(cbtevent* ev, ag* ag_src, ag* ag_dst, Player * player_src, Player * player_dst)
{
	uint16_t index = 0;
	bool correct_id = false;
	Player* current_player = nullptr;

	if (!ev) return false;
	if (!player_src && !player_dst) return false;

	if (can_block && ev->result == CBTR_BLOCK) return false;
	if (can_evade && ev->result == CBTR_EVADE) return false;
	if (can_invuln && ev->result == CBTR_ABSORB) return false;

	if (!verbosity) return false;

	for (index = 0; index < ids_size; index++)
	{
		if (ev->skillid == this->ids[index])
		{
			correct_id = true;
			break;
		}
	}

	if (!correct_id
		&& ids_size > 0) return false;

	if (frequency_global != 0
		&& ev->time < last_hit_time + frequency_global - ms_per_tick)
	{
		return false;
	}

	if (ev->is_buffremove != is_buffremove)
	{
		return false;
	}

	if (is_activation)
	{//Normal and quickness activations are interchangable
		if (is_activation == ACTV_NORMAL
			|| is_activation == ACTV_QUICKNESS)
		{
			if (ev->is_activation != ACTV_NORMAL
				&& ev->is_activation != ACTV_QUICKNESS)
			{
				return false;
			}
		}
	}

	if (is_buffremove//TODO: this check is wrong. overstack does not require buffremove
		&& overstack_value >= 0
		&& overstack_value != ev->overstack_value)
	{
		return false;
	}

	if (value != -1
		&& ev->value != value)
	{
		return false;
	}

	if (target_is_dst)
	{
		current_player = player_dst;
	}
	else
	{
		current_player = player_src;
	}

	if (!current_player) return false;

	if (!valid_if_down && current_player->is_downed) return false;

	if (is_interupt && current_player->last_stab_time > ev->time) return false;

	if (!special_requirement(*this, ev, ag_src, ag_dst, player_src, player_dst, current_player)) return false;

	last_hit_time = ev->time;

	return special_value(*this, ev, ag_src, ag_dst, player_src, player_dst, current_player);
}

std::string Mechanic::getIniName()
{
	return std::to_string(ids[0])
		+ " - " + (boss ? boss->name : "")
		+ " - " + name;
}

std::string Mechanic::getChartName()
{
	return (boss ? boss->name : "")
		+ " - " + name;
}

bool Mechanic::operator==(Mechanic* other_mechanic)
{
	return other_mechanic && ids[0] == other_mechanic->ids[0];
}

bool requirementDefault(const Mechanic &current_mechanic, cbtevent* ev, ag* ag_src, ag* ag_dst, Player * player_src, Player * player_dst, Player* current_player)
{
	return true;
}

bool requirementDhuumSnatch(const Mechanic &current_mechanic, cbtevent* ev, ag* ag_src, ag* ag_dst, Player * player_src, Player * player_dst, Player* current_player)
{
	static std::list<std::pair<uint16_t, uint64_t>> players_snatched;//pair is <instance id,last snatch time>

	for (auto current_pair = players_snatched.begin(); current_pair != players_snatched.end(); ++current_pair)
	{
		//if player has been snatched before and is in tracking
		if (ev->dst_instid == current_pair->first)
		{
			if ((current_pair->second + current_mechanic.frequency_player) > ev->time)
			{
				current_pair->second = ev->time;
				return false;
			}
			else
			{
				current_pair->second = ev->time;
				return true;
			}
		}
	}

	//if player not seen before
	players_snatched.push_back(std::pair<uint16_t, uint64_t>(ev->dst_instid, ev->time));
	return true;
}

bool requirementBuffApply(const Mechanic & current_mechanic, cbtevent* ev, ag* ag_src, ag* ag_dst, Player * player_src, Player * player_dst, Player * current_player)
{
	return ev
		&& ev->buff
		&& ev->buff_dmg == 0;
}

bool requirementKcCore(const Mechanic & current_mechanic, cbtevent* ev, ag* ag_src, ag* ag_dst, Player * player_src, Player * player_dst, Player* current_player)
{
	if (!ev) return false;

	//need player as src and agent (core) as dst
	if (!player_src) return false;
	if (!ag_dst) return false;

	//must be physical hit
	if (ev->is_statechange) return false;
	if (ev->is_activation) return false;
	if (ev->is_buffremove) return false;
	if (ev->buff) return false;

	//must be hitting kc core
	if (ag_dst->prof != 16261) return false;

	return true;
}

bool requirementShTdCc(const Mechanic & current_mechanic, cbtevent* ev, ag* ag_src, ag* ag_dst, Player * player_src, Player * player_dst, Player* current_player)
{
	if (!ev) return false;

	//need player as src and agent (TD) as dst
	if (!player_src) return false;
	if (!ag_dst) return false;

	//must be buff apply
	if (ev->is_statechange) return false;
	if (ev->is_activation) return false;
	if (ev->is_buffremove) return false;
	if (!ev->buff) return false;
	if (ev->buff_dmg) return false;

	//must be hitting a tormented dead
	if (ag_dst->prof != 19422) return false;

	return true;
}

bool requirementCaveEyeCc(const Mechanic & current_mechanic, cbtevent* ev, ag* ag_src, ag* ag_dst, Player * player_src, Player * player_dst, Player* current_player)
{
	if (!ev) return false;

	//need player as src and agent (Eye) as dst
	if (!player_src) return false;
	if (!ag_dst) return false;

	//must be buff apply
	if (ev->is_statechange) return false;
	if (ev->is_activation) return false;
	if (ev->is_buffremove) return false;
	if (!ev->buff) return false;
	if (ev->buff_dmg) return false;

	//must be hitting an eye
	if (ag_dst->prof != 0x4CC3
		&& ag_dst->prof != 0x4D84) return false;

	return true;
}

bool requirementDhuumMessenger(const Mechanic & current_mechanic, cbtevent * ev, ag * ag_src, ag * ag_dst, Player * player_src, Player * player_dst, Player * current_player)
{
	static std::list<uint16_t> messengers;
	static std::mutex messengers_mtx;

	if (!ev) return false;

	//need player as src and agent (messenger) as dst
	if (!player_src) return false;
	if (!ag_dst) return false;

	//must be physical hit
	if (ev->is_statechange) return false;
	if (ev->is_activation) return false;
	if (ev->is_buffremove) return false;
	if (ev->buff) return false;

	//must be hitting a messenger
	if (ag_dst->prof != 19807) return false;

	const auto new_messenger = ev->dst_instid;

	std::lock_guard<std::mutex> lg(messengers_mtx);

	auto it = std::find(messengers.begin(), messengers.end(), new_messenger);

	if (it != messengers.end())
	{
		return false;
	}

	messengers.push_front(new_messenger);
	return true;
}

bool requirementDeimosOil(const Mechanic &current_mechanic, cbtevent* ev, ag* ag_src, ag* ag_dst, Player * player_src, Player * player_dst, Player* current_player)
{
	static const uint16_t max_deimos_oils = 3;
	static DeimosOil deimos_oils[max_deimos_oils];

	DeimosOil* current_oil = nullptr;
	DeimosOil* oldest_oil = &deimos_oils[0];

	//find if the oil is already tracked
	for (auto index = 0; index < max_deimos_oils; index++)
	{
		if (deimos_oils[index].last_touch_time < oldest_oil->last_touch_time)//find oldest oil
		{
			oldest_oil = &deimos_oils[index];
		}
		if (deimos_oils[index].id == ev->src_instid)//if oil is already known
		{
			current_oil = &deimos_oils[index];
		}
	}

	//if oil is new
	if (!current_oil)
	{
		current_oil = oldest_oil;
		current_oil->id = ev->src_instid;
		current_oil->first_touch_time = ev->time;
		current_oil->last_touch_time = ev->time;
		return true;
	}
	else
	{//oil is already known
		current_oil->last_touch_time = ev->time;
		if ((ev->time - current_oil->last_touch_time) > current_mechanic.frequency_player)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool requirementOnSelf(const Mechanic &current_mechanic, cbtevent* ev, ag* ag_src, ag* ag_dst, Player * player_src, Player * player_dst, Player* current_player)
{
	return ev->src_instid == ev->dst_instid;
}

bool requirementOnSelfRevealedInHarvestTemple(const Mechanic& current_mechanic, cbtevent* ev,
							   ag* ag_src, ag* ag_dst, Player* player_src,
							   Player* player_dst, Player* current_player)
{
	
	if (!ev) return false;
	// In Harvest Temple
	if (!current_player->current_log_npc || !current_mechanic.boss->hasId(*current_player->current_log_npc)) return false;
	// Applying to self
	if (ev->src_instid != ev->dst_instid) return false;
	// Applying, not removing
	if (ev->is_buffremove) return false;
	// Applying buff
	if (!ev->buff || ev->buff_dmg != 0) return false;
	return true;
}

bool requirementSpecificBoss(const Mechanic& current_mechanic, cbtevent* ev,
                             ag* ag_src, ag* ag_dst, Player* player_src,
                             Player* player_dst, Player* current_player)
{
	if (!current_player->current_log_npc) return false;
	if (!current_mechanic.boss) return false;
	return current_mechanic.boss->hasId(*current_player->current_log_npc);
}

int64_t valueDefault(const Mechanic &current_mechanic, cbtevent* ev, ag* ag_src, ag* ag_dst, Player * player_src, Player * player_dst, Player* current_player)
{
	return 1;
}

int64_t valueDhuumShackles(const Mechanic & current_mechanic, cbtevent* ev, ag* ag_src, ag* ag_dst, Player * player_src, Player * player_dst, Player * current_player)
{
	return (30000 - ev->value) / 1000;
}

std::vector<Mechanic>& getMechanics()
{
	static std::vector<Mechanic>* mechanics =
		new std::vector<Mechanic>
	{
		//Vale Guardian
		Mechanic("被傳送了(Unstable Magic Spike)",{31392,31860},&boss_vg,true,false,verbosity_all,false,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Unstable Magic Spike",""),
		//I'm not sure why this mechanic has 4 ids, but it appears to for some reason
		//all these ids are for when 4 people are in the green circle
		//it appears to be a separate id for the 90% hp blast when <4 people are in the green
		//all 4 ids are called "Distributed Magic"
		Mechanic("站在綠色圓圈裡(Distributed Magic)",{31340,31391,31529,31750},&boss_vg,false,false,verbosity_chart,false,false,target_location_dst,0,0,-1,-1,ACTV_NONE,CBTB_NONE,false,false,false,requirementDefault,valueDefault,"Distributed Magic",""),

		//Gorseval
		Mechanic().setName("被衝擊(Spectral Impact)").setIds({MECHANIC_GORS_SLAM}).setIsInterupt(true).setBoss(&boss_gors),
		Mechanic().setName("被縛靈牢籠關住了(Ghastly Prison)").setIds({MECHANIC_GORS_EGG}).setBoss(&boss_gors),
		Mechanic().setName("觸碰了球(Spectral Darkness)").setIds({MECHANIC_GORS_ORB}).setBoss(&boss_gors).setSpecialRequirement(requirementBuffApply),
		
		//Sabetha
		Mechanic().setName("拿到工兵炸彈(Sapper Bomb)").setIds({MECHANIC_SAB_SAPPER_BOMB}).setFailIfHit(false).setValidIfDown(true).setBoss(&boss_sab),
		Mechanic().setName("拿到定時炸彈(Time Bomb)").setIds({MECHANIC_SAB_TIME_BOMB}).setFailIfHit(false).setValidIfDown(true).setBoss(&boss_sab),
		Mechanic().setName("站在加農砲彈幕中(Cannon Barrage)").setIds({MECHANIC_SAB_CANNON}).setBoss(&boss_sab),
		//Mechanic().setName("touched the flame wall").setIds({MECHANIC_SAB_FLAMEWALL}).setBoss(&boss_sab),

		//Slothasor
		Mechanic().setName("被衝擊波擊中(Tantrum)").setIds({MECHANIC_SLOTH_TANTRUM}).setBoss(&boss_sloth),
		Mechanic().setName("拿到劇烈毒素(Volatile Poison)").setIds({MECHANIC_SLOTH_BOMB}).setFailIfHit(false).setFrequencyPlayer(6000).setBoss(&boss_sloth),
		Mechanic().setName("站在毒素法池中(Volatile Poison)").setIds({MECHANIC_SLOTH_BOMB_AOE}).setVerbosity(verbosity_chart).setBoss(&boss_sloth),
		Mechanic().setName("被污濁口氣擊中(Halitosis)").setIds({MECHANIC_SLOTH_FLAME_BREATH}).setBoss(&boss_sloth),
		Mechanic().setName("被孢子釋放擊中(Spore Release)").setIds({MECHANIC_SLOTH_SHAKE}).setBoss(&boss_sloth),
		Mechanic().setName("被注視").setIds({MECHANIC_SLOTH_FIXATE}).setFailIfHit(false).setBoss(&boss_sloth),

		//Bandit Trio
		Mechanic("丟了蜂巢",{34533},&boss_trio,false,false,verbosity_all,false,false,target_location_src,0,0,-1,-1,ACTV_NORMAL,CBTB_NONE,false,false,false,requirementDefault,valueDefault,"Beehive",""),
		Mechanic("丟了油桶",{34471},&boss_trio,false,false,verbosity_all,false,false,target_location_src,0,0,-1,-1,ACTV_NORMAL,CBTB_NONE,false,false,false,requirementDefault,valueDefault,"Throw",""),

		//Matthias
		//Mechanic().setName("was hadoukened").setIds({MECHANIC_MATT_HADOUKEN_HUMAN,MECHANIC_MATT_HADOUKEN_ABOM}).setBoss(&boss_matti),
		Mechanic().setName("反射鲜血碎片(Blood Shards)").setIds({MECHANIC_MATT_SHARD_HUMAN,MECHANIC_MATT_SHARD_ABOM}).setTargetIsDst(false).setBoss(&boss_matti),
		Mechanic().setName("拿到不稳定的血魔法(Unstable Blood Magic)").setIds({MECHANIC_MATT_BOMB}).setFailIfHit(false).setFrequencyPlayer(12000).setBoss(&boss_matti),
		Mechanic().setName("拿到腐化(Corruption)").setIds({MECHANIC_MATT_CORRUPTION}).setFailIfHit(false).setBoss(&boss_matti),
		Mechanic().setName("被犧牲").setIds({MECHANIC_MATT_SACRIFICE}).setFailIfHit(false).setBoss(&boss_matti),
		Mechanic("觸碰了鬼魂(Surrender)",{34413},&boss_matti,true,false,verbosity_all,false,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Surrender",""),
		//Mechanic("touched an icy patch",{26766},&boss_matti,true,false,verbosity_all,false,true,target_location_dst,2000,0,-1,10000,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Slow",""), //look for Slow application with 10 sec duration. Disabled because some mob in Istan applies the same duration of slow
		Mechanic("站在熾熱渦流中(Fiery Vortex)",{34466},&boss_matti,true,false,verbosity_all,false,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Fiery Vortex",""),
		Mechanic("站在雷電中(Thunder)",{34543},&boss_matti,true,false,verbosity_all,false,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Thunder",""),

		//Keep Construct
		Mechanic().setName("被盯上了").setIds({MECHANIC_KC_FIXATE}).setFailIfHit(false).setBoss(&boss_kc),
		//Mechanic().setName("is west fixated").setIds({MECHANIC_KC_FIXATE_WEST}).setFailIfHit(false).setBoss(&boss_kc),
		Mechanic().setName("碰觸到核心").setFailIfHit(false).setTargetIsDst(false).setFrequencyPlayer(8000).setBoss(&boss_kc).setSpecialRequirement(requirementKcCore),
		Mechanic("被落體衝擊打倒(Tower Drop)",{35086},&boss_kc,true,false,verbosity_all,true,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Tower Drop",""),
		Mechanic("站在幻影劈斬裡(Phantasmal Blades)",{35137,34971,35086},&boss_kc,true,false,verbosity_all,false,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Phantasmal Blades",""),
		
		//Xera
		Mechanic("站在半圓形紅圈裡",{34921},&boss_xera,true,false,verbosity_all,false,true,target_location_dst,4000,0,-1,-1,ACTV_NONE,CBTB_NONE,false,false,true,requirementDefault,valueDefault,"TODO:check internal name",""),
		Mechanic().setName("掌握了環境魔法(Intervention)").setIds({MECHANIC_XERA_MAGIC}).setFailIfHit(false).setValidIfDown(true).setValue(15000).setBoss(&boss_xera),
		Mechanic().setName("使用了環境魔法(Intervention)").setIds({MECHANIC_XERA_MAGIC_BUFF}).setFailIfHit(false).setTargetIsDst(false).setFrequencyGlobal(12000).setValidIfDown(true).setBoss(&boss_xera).setSpecialRequirement(requirementOnSelf).setVerbosity(0),
		Mechanic().setName("觸發了時空碎片(Temporal Shred)").setIds({MECHANIC_XERA_ORB}).setBoss(&boss_xera),
		Mechanic().setName("站在時間漩渦中(Temporal Vortex)").setIds({MECHANIC_XERA_ORB_AOE}).setFrequencyPlayer(1000).setVerbosity(verbosity_chart).setBoss(&boss_xera),
		Mechanic().setName("被傳送").setIds({MECHANIC_XERA_PORT}).setVerbosity(verbosity_chart).setBoss(&boss_xera),

		//Cairn
		Mechanic().setName("被錯位傳送(Displacement)").setIds({MECHANIC_CAIRN_TELEPORT}).setBoss(&boss_cairn),
		Mechanic().setName("被迴旋掃蕩擊中(Orbital Sweep)").setIds({MECHANIC_CAIRN_SWEEP}).setBoss(&boss_cairn).setIsInterupt(true),
		//Mechanic().setName("reflected shards").setIds({MECHANIC_CAIRN_SHARD}).setTargetIsDst(false).setBoss(&boss_cairn),
		Mechanic().setName("錯過了綠圈而被空間操縱(Spatial Manipulation)").setIds({MECHANIC_CAIRN_GREEN_A,MECHANIC_CAIRN_GREEN_B,MECHANIC_CAIRN_GREEN_C,MECHANIC_CAIRN_GREEN_D,MECHANIC_CAIRN_GREEN_E,MECHANIC_CAIRN_GREEN_F}).setIsInterupt(true).setBoss(&boss_cairn),
		
		//Samarog
		Mechanic().setName("被震盪波擊中(Shockwave)").setIds({MECHANIC_SAM_SHOCKWAVE}).setIsInterupt(true).setBoss(&boss_sam),
		Mechanic().setName("被猛擊囚徒擊中(水平攻擊)(Prisoner Sweep)").setIds({MECHANIC_SAM_SLAP_HORIZONTAL}).setIsInterupt(true).setBoss(&boss_sam),
		Mechanic().setName("被巨棒揮舞擊中(垂直攻擊)(Bludgeon)").setIds({MECHANIC_SAM_SLAP_VERTICAL}).setIsInterupt(true).setBoss(&boss_sam),
		Mechanic().setName("被固定住了").setIds({MECHANIC_SAM_FIXATE_SAM}).setFailIfHit(false).setBoss(&boss_sam),
		Mechanic().setName("拿到大綠圈").setIds({MECHANIC_SAM_GREEN_BIG}).setFailIfHit(false).setBoss(&boss_sam),
		Mechanic().setName("拿到小綠圈").setIds({MECHANIC_SAM_GREEN_SMALL}).setFailIfHit(false).setBoss(&boss_sam),

		//Deimos
		Mechanic("碰到油圈(Rapid Decay)",{37716},&boss_deimos,true,false,verbosity_all,false,true,target_location_dst,5000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDeimosOil,valueDefault,"Rapid Decay",""),
		Mechanic().setName("被湮滅/惡魔衝擊波擊退了(Annihilate, Demonic Shock Wave)").setIds({MECHANIC_DEIMOS_SMASH,MECHANIC_DEIMOS_SMASH_INITIAL,MECHANIC_DEIMOS_SMASH_END_A,MECHANIC_DEIMOS_SMASH_END_B}).setBoss(&boss_deimos),
		Mechanic().setName("清除惡魔之淚(Demonic Projection)").setIds({MECHANIC_DEIMOS_TEAR}).setFailIfHit(false).setBoss(&boss_deimos),
		Mechanic("被珍瑟之眼選擇了(傳送綠圈)",{37730},&boss_deimos,false,true,verbosity_all,false,false,target_location_dst,0,0,-1,-1,ACTV_NONE,CBTB_NONE,false,false,false,requirementDefault,valueDefault,"Chosen by Eye of Janthir",""),
		Mechanic("被傳送了",{38169},&boss_deimos,false,true,verbosity_chart,false,false,target_location_dst,0,0,-1,-1,ACTV_NONE,CBTB_NONE,false,false,false,requirementDefault,valueDefault,"",""),

		//Soulless Horror
		Mechanic().setName("站在渦流打擊內圈裡(Vortex Slash)").setIds({MECHANIC_HORROR_DONUT_INNER}).setVerbosity(verbosity_chart).setBoss(&boss_sh),
		Mechanic().setName("站在渦流打擊外圈裡(Vortex Slash)").setIds({MECHANIC_HORROR_DONUT_OUTER}).setVerbosity(verbosity_chart).setBoss(&boss_sh),
		Mechanic().setName("站在靈魂裂隙裡(Soul Rift)").setIds({MECHANIC_HORROR_GOLEM_AOE}).setBoss(&boss_sh),
		Mechanic().setName("站在四重裂擊範圍裡(Quad Slash)").setIds({MECHANIC_HORROR_PIE_4_A,MECHANIC_HORROR_PIE_4_B}).setVerbosity(verbosity_chart).setBoss(&boss_sh),
		Mechanic().setName("碰到旋轉鐮刀(Spinning Slash)").setIds({MECHANIC_HORROR_SCYTHE}).setBoss(&boss_sh),
		Mechanic().setName("發起挑戰而被注視(Issue Challenge)").setIds({MECHANIC_HORROR_FIXATE}).setFailIfHit(false).setVerbosity(verbosity_chart).setBoss(&boss_sh),
		Mechanic().setName("累積崩潰壞死層數(Necrosis)").setIds({MECHANIC_HORROR_DEBUFF}).setFailIfHit(false).setVerbosity(verbosity_chart).setBoss(&boss_sh),
		Mechanic("使用CC在磨難死者上",{872,833,31465},&boss_sh,true,true,verbosity_all,false,true,target_location_src,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,false,false,false,requirementShTdCc,valueDefault,"Stun, Daze, Temporal stasis",""),

		//Statues
		Mechanic().setName("被吐了，沾染上飢渴瘴氣(Hungering Miasma)").setIds({MECHANIC_EATER_PUKE}).setFrequencyPlayer(3000).setVerbosity(verbosity_chart).setBoss(&boss_soul_eater),
		Mechanic().setName("站在蛛網上(Spinner's Web)").setIds({MECHANIC_EATER_WEB}).setFrequencyPlayer(3000).setVerbosity(verbosity_chart).setBoss(&boss_soul_eater),
		Mechanic().setName("拿到光球").setIds({MECHANIC_EATER_ORB}).setFrequencyPlayer(ms_per_tick).setFailIfHit(false).setBoss(&boss_soul_eater),
		Mechanic().setName("丟光球(Reclaimed Energy)").setNameInternal("Reclaimed Energy").setIds({47942}).setTargetIsDst(false).setIsActivation(ACTV_NORMAL).setFailIfHit(false).setBoss(&boss_soul_eater),
		Mechanic("接住冰雹綠圈",{47013},&boss_ice_king,false,true,verbosity_chart,false,false,target_location_dst,0,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Hailstorm",""),
		Mechanic("使用CC在眼睛上",{872},&boss_cave,false,true,verbosity_all,false,false,target_location_src,0,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementCaveEyeCc,valueDefault,"Stun",""),

		//Dhuum
		Mechanic().setName("碰到了德姆信使(Hateful Ephemera)").setIds({MECHANIC_DHUUM_GOLEM}).setBoss(&boss_dhuum),
		Mechanic().setName("被靈魂桎梏(Soul Shackle)").setIds({MECHANIC_DHUUM_SHACKLE}).setFailIfHit(false).setTargetIsDst(false).setBoss(&boss_dhuum),
		Mechanic().setName("被靈魂桎梏(Soul Shackle)").setIds({MECHANIC_DHUUM_SHACKLE}).setFailIfHit(false).setBoss(&boss_dhuum),
		//Mechanic().setName("popped shackles").setIds({MECHANIC_DHUUM_SHACKLE}).setFailIfHit(false).setIsBuffremove(CBTB_MANUAL).setTargetIsDst(false).setSpecialValue(valueDhuumShackles).setBoss(&boss_dhuum),
		//Mechanic().setName("popped shackles").setIds({MECHANIC_DHUUM_SHACKLE}).setFailIfHit(false).setIsBuffremove(CBTB_MANUAL).setSpecialValue(valueDhuumShackles).setBoss(&boss_dhuum),
		Mechanic().setName("被感染了").setIds({MECHANIC_DHUUM_AFFLICTION}).setFrequencyPlayer(13000 + ms_per_tick).setFailIfHit(false).setValidIfDown(true).setBoss(&boss_dhuum),
		Mechanic("使用圓弧苦難(Arcing Affliction)",{48121},&boss_dhuum,false,true,verbosity_chart,false,false,target_location_dst,0,0,-1,-1,ACTV_NONE,CBTB_NONE,false,false,false,requirementDefault,valueDefault,"Arcing Affliction",""),
		Mechanic().setName("站在裂縫中(Cull)").setIds({MECHANIC_DHUUM_CRACK}).setBoss(&boss_dhuum),
		Mechanic().setName("站在腐敗炸彈上(Putrid Bomb)").setIds({MECHANIC_DHUUM_MARK}).setVerbosity(verbosity_chart).setBoss(&boss_dhuum),
		Mechanic().setName("被災變循環吸到中間(Cataclysmic Cycle)").setIds({MECHANIC_DHUUM_SUCK_AOE}).setBoss(&boss_dhuum),
		Mechanic().setName("站在死亡標記中(Death Mark)").setIds({MECHANIC_DHUUM_TELEPORT_AOE}).setBoss(&boss_dhuum),
		//Mechanic().setName("died on green").setIds({MECHANIC_DHUUM_GREEN_TIMER}).setIsBuffremove(CBTB_MANUAL).setOverstackValue(0).setBoss(&boss_dhuum),
		//Mechanic().setName("aggroed a messenger").setNameInternal("").setTargetIsDst(false).setFailIfHit(false).setFrequencyPlayer(0).setValidIfDown(true).setBoss(&boss_dhuum).setSpecialRequirement(requirementDhuumMessenger),
		Mechanic().setName("的靈魂被剝離軀體").setIds({MECHANIC_DHUUM_SNATCH}).setSpecialRequirement(requirementDhuumSnatch).setBoss(&boss_dhuum),
		//Mechanic().setName("canceled button channel").setIds({MECHANIC_DHUUM_BUTTON_CHANNEL}).setIsActivation(ACTV_CANCEL_CANCEL).setBoss(&boss_dhuum),
		Mechanic().setName("被尖爪拉過去(Slash)").setIds({MECHANIC_DHUUM_CONE}).setBoss(&boss_dhuum),

		//Conjured Amalgamate
		Mechanic().setName("被粉碎重擊打倒(Pulverize)").setIds({MECHANIC_AMAL_SQUASH}).setIsInterupt(true).setBoss(&boss_ca),
		Mechanic().setName("使用咒術巨劍").setNameInternal("Conjured Greatsword").setIds({52325}).setTargetIsDst(false).setIsActivation(ACTV_NORMAL).setFailIfHit(false).setBoss(&boss_ca),
		Mechanic().setName("使用咒術防護").setNameInternal("Conjured Protection").setIds({52780}).setTargetIsDst(false).setIsActivation(ACTV_NORMAL).setFailIfHit(false).setBoss(&boss_ca),

		//Twin Largos Assasins
		Mechanic().setName("被大海湧動擊退(Sea Swell)").setIds({MECHANIC_LARGOS_SHOCKWAVE}).setIsInterupt(true).setBoss(&boss_largos),
		Mechanic().setName("被浸水(Waterlogged)").setIds({MECHANIC_LARGOS_WATERLOGGED}).setVerbosity(verbosity_chart).setValidIfDown(true).setFrequencyPlayer(1).setBoss(&boss_largos),
		Mechanic().setName("被水棲阻攔包裹(Aquatic Detainment)").setIds({MECHANIC_LARGOS_BUBBLE}).setBoss(&boss_largos),
		Mechanic().setName("拿到潮水法池(Tidal Pool)").setIds({MECHANIC_LARGOS_TIDAL_POOL}).setFailIfHit(false).setBoss(&boss_largos),
		Mechanic().setName("站在間歇溫泉裡(Geyser)").setIds({MECHANIC_LARGOS_GEYSER}).setBoss(&boss_largos),
		Mechanic().setName("被蒸氣突進擊中(Vapor Rush)").setIds({MECHANIC_LARGOS_DASH}).setBoss(&boss_largos),
		Mechanic().setName("被蒸汽噴發奪取增益(Vapor Jet)").setIds({MECHANIC_LARGOS_BOON_RIP}).setBoss(&boss_largos),
		Mechanic().setName("站在水棲渦流裡(Aquatic Vortex)").setIds({MECHANIC_LARGOS_WHIRLPOOL}).setBoss(&boss_largos),

		//Qadim
		Mechanic().setName("被火焰浪潮擊退(Fire Wave)").setIds({MECHANIC_QADIM_SHOCKWAVE_A,MECHANIC_QADIM_SHOCKWAVE_B}).setBoss(&boss_qadim),
		Mechanic().setName("站在火焰之舞中(Fire Dance)").setIds({MECHANIC_QADIM_ARCING_FIRE_A,MECHANIC_QADIM_ARCING_FIRE_B,MECHANIC_QADIM_ARCING_FIRE_C}).setVerbosity(verbosity_chart).setBoss(&boss_qadim),
		//Mechanic().setName("stood in giant fireball").setIds({MECHANIC_QADIM_BOUNCING_FIREBALL_BIG_A,MECHANIC_QADIM_BOUNCING_FIREBALL_BIG_B,MECHANIC_QADIM_BOUNCING_FIREBALL_BIG_C}).setBoss(&boss_qadim),
		Mechanic().setName("被移形換位(Swap)").setIds({MECHANIC_QADIM_TELEPORT}).setBoss(&boss_qadim).setValidIfDown(true),
		Mechanic().setName("站在卡迪姆腳下(Sea of Flame)").setNameInternal("Sea of Flame").setIds({52461}).setBoss(&boss_qadim),

		//Adina
		Mechanic("被耀光目盲(Radiant Blindness)",{56593},&boss_adina,false,false,verbosity_chart,false,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Radiant Blindness",""),
		Mechanic("面向鑽石圍壁(Diamond Palisade)", {56114}, &boss_adina, false, false, verbosity_all, false, true, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE,true, false, true,requirementDefault, valueDefault, "Diamond Palisade", ""),
		Mechanic("碰到地殼劇變(Tectonic Upheaval)",{56558},&boss_adina,true,false,verbosity_all,false,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Tectonic Upheaval",""),
		Mechanic("碰到石筍(Stalagmites)",{56141},&boss_adina,true,false,verbosity_all,false,true,target_location_dst,1000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Stalagmites",""),
		//Mechanic("has pillar",{47860},&boss_adina,false,false,verbosity_all,false,false,target_location_dst,0,0,-1,-1,ACTV_NONE,CBTB_NONE,false,false,false,requirementDefault,valueDefault,"",""),//wrong id?
		
		//Sabir
		Mechanic("碰到恐怖氣流(Dire Drafts)",{56202},&boss_sabir,true,false,verbosity_all,false,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Dire Drafts",""),
		Mechanic("被放縱風暴擊倒地(Unbridled Tempest)",{56643},&boss_sabir,true,false,verbosity_all,false,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,false,false,true,requirementDefault,valueDefault,"Unbridled Tempest",""),
		Mechanic("不在風暴中心而被風暴狂怒轟炸(Fury of the Storm)",{56372},&boss_sabir,true,false,verbosity_all,false,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,false,false,false,requirementDefault,valueDefault,"Fury of the Storm",""),
		Mechanic("被宏大颶風吹退(Walloping Wind)", {56094},&boss_sabir,true,false,verbosity_chart,false,false,target_location_dst,0,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Walloping Wind",""),

		//Qadim the Peerless
		Mechanic("被注視",{56510},&boss_qadim2,false,true,verbosity_all,false,false,target_location_dst,0,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Fixated",""),
		Mechanic("碰到殘餘能量(Residual Impact, Pylon Debris Field)", {56180,56378,56541},&boss_qadim2,true,false,verbosity_all,false,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Residual Impact, Pylon Debris Field",""),//ids are big,small(CM),pylon
		Mechanic("被烙印風暴閃電擊中(Brandstorm Lightning)",{56656},&boss_qadim2,true,false,verbosity_chart,false,false,target_location_dst,1000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Brandstorm Lightning",""),		
		Mechanic("被混沌鎖鏈擊中(Rain of Chaos)",{56527},&boss_qadim2,true,false,verbosity_all,false,false,target_location_dst,1000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Rain of Chaos",""),
		Mechanic("碰到混沌呼喚(Chaos Called)",{56145},&boss_qadim2,true,false,verbosity_all,false,false,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Chaos Called",""),
		Mechanic("被反彈斥力擊退(Force of Retaliation)",{56134},&boss_qadim2,true,false,verbosity_all,true,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Force of Retaliation",""),
		Mechanic("碰到浩劫之力(地毯)(Force of Havoc)",{56441},&boss_qadim2,true,false,verbosity_chart,false,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Force of Havoc",""),
		Mechanic("被閃擊重創碾過(Battering Blitz)",{56616},&boss_qadim2,true,false,verbosity_all,false,true,target_location_dst,2000,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Battering Blitz",""),
		Mechanic("被混沌侵蝕了(Caustic Chaos)",{56332},&boss_qadim2,true,true,verbosity_all,false,false,target_location_dst,100,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Caustic Chaos",""),
		Mechanic("被腐蝕混沌擊中(Caustic Chaos)",{56543},&boss_qadim2,false,true,verbosity_chart,false,false,target_location_dst,100,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"Caustic Chaos",""),
		//Mechanic("has lightning", {51371},&boss_qadim2,false,true,verbosity_all,false,false,target_location_dst,0,0,-1,-1,ACTV_NONE,CBTB_NONE,true,true,true,requirementDefault,valueDefault,"",""),

		//Fractals of the Mist Instabilities
		Mechanic().setName("拿到流體炸彈(Flux Bomb)").setIds({MECHANIC_FOTM_FLUX_BOMB}).setFailIfHit(false).setBoss(&boss_fotm_generic),

		//MAMA
		//Mechanic().setName("vomited on someone").setIds({MECHANIC_NIGHTMARE_VOMIT}).setTargetIsDst(false).setBoss(&boss_fotm_generic),
		Mechanic().setName("被衝擊波擊退(Blastwave)").setIds({MECHANIC_MAMA_WHIRL,MECHANIC_MAMA_WHIRL_NORMAL}).setBoss(&boss_mama),
		Mechanic().setName("被發脾氣擊退(Tantrum)").setIds({MECHANIC_MAMA_KNOCK}).setBoss(&boss_mama),
		Mechanic().setName("被跳上(Leap)").setIds({MECHANIC_MAMA_LEAP}).setBoss(&boss_mama),
		Mechanic().setName("站在毒圈裡(Shoot)").setIds({MECHANIC_MAMA_ACID}).setBoss(&boss_mama),
		Mechanic().setName("被騎士擊倒").setIds({MECHANIC_MAMA_KNIGHT_SMASH}).setBoss(&boss_mama),

		//Siax
		Mechanic().setName("站在毒圈裡(Vile Spit)").setIds({MECHANIC_SIAX_ACID}).setBoss(&boss_siax),

		//Ensolyss
		Mechanic().setName("被猛衝擊退(Lunge)").setIds({MECHANIC_ENSOLYSS_LUNGE}).setBoss(&boss_ensolyss),
		Mechanic().setName("被上聳揮擊打中(Upswing)").setIds({MECHANIC_ENSOLYSS_SMASH}).setBoss(&boss_ensolyss),

		//Arkk
		Mechanic().setName("站在地平線打擊範圍上(Horizon Strike)").setIds({MECHANIC_ARKK_PIE_A,MECHANIC_ARKK_PIE_B,MECHANIC_ARKK_PIE_C}).setBoss(&boss_arkk),
		//Mechanic().setName("was feared").setIds({MECHANIC_ARKK_FEAR}),
		Mechanic().setName("被迎頭一擊(Overhead Smash)").setIds({MECHANIC_ARKK_OVERHEAD_SMASH}).setBoss(&boss_arkk),
		Mechanic().setName("拿到炸彈").setIds({MECHANIC_ARKK_BOMB}).setFailIfHit(false).setValidIfDown(true).setBoss(&boss_arkk),//TODO Add BOSS_ARTSARIIV_ID and make boss id a vector
		Mechanic().setName("拿到綠圈").setIds({39268}).setNameInternal("Cosmic Meteor").setFailIfHit(false).setValidIfDown(true).setBoss(&boss_arkk),
		//Mechanic().setName("didn't block the goop").setIds({MECHANIC_ARKK_GOOP}).setBoss(&boss_arkk).setCanEvade(false),

		//Sorrowful Spellcaster
		Mechanic("站在元素迴旋中(Elemental Whirl)", {61463}, &boss_ai, true, false, verbosity_all, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Elemental Whirl", ""),
		//Wind
		Mechanic("被燦爛光球擊中(Fulgor Sphere)", {61487,61565}, &boss_ai, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Fulgor Sphere", ""),
		Mechanic("被元素掌控擊中(Elemental Manipulation)", {61574}, &boss_ai, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Elemental Manipulation", ""),
		//Mechanic("was launched in the air", {61205}, &boss_ai, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Wind Burst", ""),
		//Mechanic("stood in wind", {61470}, & boss_ai, false, false, verbosity_chart, false, true, target_location_dst, 5000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Volatile Wind", ""),
		//Mechanic("was hit by lightning", {61190}, &boss_ai, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Call of Storms", ""),
		//Fire
		Mechanic("被動盪烈焰擊中(Roiling Flames)", {61273,61582}, &boss_ai, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Roiling Flames", ""),
		//TODO: Mechanic("was hit by fire blades", {}, &boss_ai, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Elemental Manipulation", ""),
		//Mechanic("stood in fire", {61548}, & boss_ai, false, false, verbosity_all, false, true, target_location_dst, 5000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Volatile Fire", ""),
		Mechanic("被Boss召喚的流星擊中(Call Meteor)", {61348,61439}, &boss_ai, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Call Meteor", ""),
		Mechanic("被火焰風暴擊中(Firestorm)", {61445}, &boss_ai, true, false, verbosity_all, false, false, target_location_dst, 1000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Firestorm", ""),
		//Water
		Mechanic("被奔流箭矢擊中(Torrential Bolt)", {61349,61177}, &boss_ai, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Torrential Bolt", ""),
		//TODO: Mechanic("was hit by water blades", {}, &boss_ai, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Elemental Manipulation", ""),
		//Mechanic("stood in water", {61419}, & boss_ai, false, false, verbosity_all, false, true, target_location_dst, 5000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Volatile Waters", ""),
		//Dark
		Mechanic("被聚焦怒火擊中(雷射)(Focused Wrath)", {61344,61499}, &boss_ai, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Focused Wrath", ""),
		//TODO: Mechanic("was hit by a laser blade", {}, &boss_ai, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Empathic Manipulation", ""),
		//TODO: Mechanic("was stunned by fear", {}, &boss_ai, false, false, verbosity_all, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "", ""),

		//Icebrood Construct
		Mechanic("被寒冰波動擊中(Ice Shock Wave)", {57948,57472,57779}, &boss_icebrood_construct, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Ice Shock Wave", ""),
		Mechanic("被冰臂揮舞擊中(Ice Arm Swing)", {57516}, &boss_icebrood_construct, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Ice Arm Swing", ""),
		Mechanic("被寒冰粉碎者擊中(Ice Shatter)", {57690}, &boss_icebrood_construct, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Ice Shatter", ""),
		Mechanic("被冰晶擊中(Ice Crystal)", {57663}, &boss_icebrood_construct, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Ice Crystal", ""),
		Mechanic("被寒冰連枷擊中(Ice Flail)", {57678,57463}, &boss_icebrood_construct, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Ice Flail", ""),
		Mechanic("被致命寒冰波動擊中(Deadly Ice Shock Wave)", {57832}, &boss_icebrood_construct, true, false, verbosity_all, true, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Deadly Ice Shock Wave", ""),
		Mechanic("被粉碎手臂擊中(Shatter Arm)", {57729}, &boss_icebrood_construct, true, false, verbosity_all, true, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Shatter Arm", ""),
		
		//Voice and Claw
		Mechanic("被巨錘猛擊打中(Hammer Slam)", {58150}, &boss_voice_and_claw, false, false, verbosity_all, true, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Hammer Slam", ""),
		Mechanic("被巨錘迴旋擊退(Hammer Spin)", {58132}, &boss_voice_and_claw, false, false, verbosity_all, true, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Hammer Spin", ""),
		//not working Mechanic("was trapped", {727}, &boss_voice_and_claw, false, false, verbosity_all, false, true, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Ghostly Grasp", ""), 

		//Fraenir of Jormag
		Mechanic("被冰震擊中(Icequake)", {58811}, &boss_fraenir, false, false, verbosity_all, true, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Icequake", ""),
		Mechanic("被凍結飛彈擊飛(Frozen Missle)", {58520}, &boss_fraenir, true, false, verbosity_all, true, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Frozen Missle", ""),
		Mechanic("被寒冰激流凍結(Torrent of Ice)", {58376}, &boss_fraenir, true, false, verbosity_all, false, true, target_location_dst, 10000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Torrent of Ice", ""),
		Mechanic("被寒冰衝擊波擊中(Ice Shock Wave)", {58740}, &boss_fraenir, false, false, verbosity_all, true, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Ice Shock Wave", ""),
		Mechanic("被震動碾壓擊中(Seismic Crush)", {58518}, &boss_fraenir, false, false, verbosity_all, true, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Seismic Crush", ""),
		Mechanic("被寒冰連枷擊中(Ice Flail)", {58647,58501}, &boss_fraenir, false, false, verbosity_all, true, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Ice Flail", ""),
		Mechanic("被粉碎手臂擊中(Shatter Arm)", {58515}, &boss_fraenir, false, false, verbosity_all, true, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Shatter Arm", ""),

		//Boneskinner
		Mechanic("站在抬腳紅圈之中(Grasp)", {58233}, &boss_boneskinner, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Grasp", ""),
		Mechanic("被衝鋒擊中(Charge)", {58851}, &boss_boneskinner, true, false, verbosity_all, true, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Charge", ""),
		Mechanic("被死亡之風吹中(Death Wind)", {58546}, &boss_boneskinner, true, false, verbosity_all, true, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Death Wind", ""),

		//Whisper of Jormag
		Mechanic("被寒冰碎片擊中(Icy Slice)", {59076}, &boss_whisper, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Icy Slice", ""),
		Mechanic("被冰寒暴風擊中(Ice Tempest)", {59255}, &boss_whisper, false, false, verbosity_chart, false, true, target_location_dst, 5000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Ice Tempest", ""),
		Mechanic("有寒霜鎖鏈(Chains of Frost)", {59120}, &boss_whisper, false, true, verbosity_all, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Chains of Frost", ""),
		Mechanic("被冰霜之鍊擊中(Chains of Frost)", {59159}, &boss_whisper, true, true, verbosity_all, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Chains of Frost", ""),
		Mechanic("被自己的蔓延寒冰擊中(Spreading Ice)", {59102}, & boss_whisper, true, false, verbosity_all, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Spreading Ice", ""),
		Mechanic("被別人的蔓延寒冰擊中(Spreading Ice)", {59468}, &boss_whisper, true, false, verbosity_all, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Spreading Ice", ""),
		Mechanic("被致命聚合傷害(Lethal Coalescence)", {59441}, &boss_whisper, false, false, verbosity_all, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Lethal Coalescence", ""),

		//Cold War
		//too many procs Mechanic("was hit by icy echoes", {60354}, &boss_coldwar, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Icy Echoes", ""), 
		//not working Mechanic("was frozen", {60371}, &boss_coldwar, true, false, verbosity_all, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Flash Freeze", ""), 
		Mechanic("被刺客擊中(Call Assassins)", {60308}, &boss_coldwar, true, false, verbosity_all, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Call Assassins", ""),
		Mechanic("站在烈焰之牆裡(Flame Wall)", {60171}, &boss_coldwar, false, false, verbosity_chart, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Flame Wall", ""),
		Mechanic("被衝鋒陷陣碾過(Charge!)", {60132}, &boss_coldwar, true, false, verbosity_all, true, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Charge!", ""),
		Mechanic("被爆炸擊中(Detonate)", {60006}, & boss_coldwar, true, false, verbosity_all, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Detonate", ""),
		Mechanic("被致命聚合傷害(Lethal Coalescence)", {60545}, &boss_coldwar, false, false, verbosity_all, false, false, target_location_dst, 2000, 0, -1, -1, ACTV_NONE, CBTB_NONE, true, true, true, requirementDefault, valueDefault, "Lethal Coalescence", ""),

		//Mai Trin
		Mechanic().setName("被夢魘齊射擊中(Nightmare Fusillade)").setIds({65749}).setBoss(&boss_mai_trin),
		Mechanic().setName("被側邊夢魘齊射擊中(Nightmare Fusillade)").setIds({66089}).setBoss(&boss_mai_trin),
		Mechanic().setName("被折磨浪潮擊中(Tormenting Wave)").setIds({65031, 67866}).setBoss(&boss_mai_trin),
		Mechanic().setName("站在魔徑穿透裡").setIds({64044, 67832}).setBoss(&boss_mai_trin),
		Mechanic().setName("被萬變混沌擊中").setIds({66568}).setBoss(&boss_mai_trin),
		Mechanic().setName("受到暴露疊層").setIds({64936}).setSpecialRequirement(requirementSpecificBoss).setBoss(&boss_mai_trin),
		Mechanic().setName("被選為綠圈").setFailIfHit(false).setIds({65900, 67831}).setBoss(&boss_mai_trin),
		Mechanic().setName("受到光子飽和(不能再站綠圈)").setFailIfHit(false).setIds({67872}).setBoss(&boss_mai_trin),
		Mechanic().setName("因綠圈而倒地").setIds({67954}).setBoss(&boss_mai_trin),
		Mechanic().setName("被選為放激光").setFailIfHit(false).setIds({67856}).setBoss(&boss_mai_trin),

		//Ankka
		Mechanic().setName("被抓攫之懼擊中(Grasping Horror)").setIds({66246}).setBoss(&boss_ankka),
		Mechanic().setName("被死亡之擁(Death's Embrace)").setIds({67160}).setBoss(&boss_ankka),
		Mechanic().setName("被死亡之手擊中(Death's Hand)").setIds({66728}).setBoss(&boss_ankka),
		Mechanic().setName("被恐懼之牆擊中(Wall of Fear)").setIds({66824}).setBoss(&boss_ankka),
		Mechanic().setName("被折磨波潮擊中(Wave of Torment)").setIds({64669}).setBoss(&boss_ankka),

		//Minister Li
		Mechanic().setName("被巨龍劈砍－震波擊中(Dragon Slash—Wave)").setIds({64952, 67825}).setBoss(&boss_minister_li),
		Mechanic().setName("被巨龍劈砍－爆發擊中(Dragon Slash—Burst)").setIds({66465, 65378}).setBoss(&boss_minister_li),
		Mechanic().setName("被巨龍劈砍－衝刺擊中(Dragon Slash—Rush)").setIds({66090, 64619, 67824, 67943}).setBoss(&boss_minister_li),
		Mechanic().setName("被標記驅逐(橘圈)(Targeted Expulsion)").setIds({64277, 67982}).setBoss(&boss_minister_li),
		Mechanic().setName("被選為綠圈").setIds({64300, 68004}).setBoss(&boss_minister_li),
		Mechanic().setName("站在急速正義的火焰軌跡裡(Rushing Justice)").setIds({65608, 68028}).setBoss(&boss_minister_li),
		Mechanic().setName("重疊了爆炸指令(Booming Command)").setIds({65243}).setBoss(&boss_minister_li),
		Mechanic().setName("被注視").setIds({66140}).setSpecialRequirement(requirementSpecificBoss).setBoss(&boss_minister_li),
		Mechanic().setName("被利劍風暴擊中(Storm of Swords)").setIds({63838, 63550, 65569}).setBoss(&boss_minister_li),
		Mechanic().setName("被翠玉破壞砲擊中(Jade Buster Cannon)").setIds({64016}).setBoss(&boss_minister_li),
		Mechanic().setName("受到衰弱疊層").setIds({67972}).setSpecialRequirement(requirementSpecificBoss).setBoss(&boss_minister_li),
		Mechanic().setName("受到虛弱疊層").setIds({67965}).setSpecialRequirement(requirementSpecificBoss).setBoss(&boss_minister_li),
		Mechanic().setName("受到暴露疊層").setIds({64936}).setSpecialRequirement(requirementSpecificBoss).setBoss(&boss_minister_li),


		//Harvest Temple
		Mechanic().setName("受到虛空的影響(Influence of the Void)").setIds({64524}).setBoss(&boss_the_dragonvoid),
		Mechanic().setName("被虛空擊中").setIds({66566}).setBoss(&boss_the_dragonvoid),
		Mechanic().setName("被卓瑪之息擊中").setIds({65517, 66216, 67607}).setBoss(&boss_the_dragonvoid),
		Mechanic().setName("被派莫德斯的猛擊打中").setIds({64527}).setBoss(&boss_the_dragonvoid),
		Mechanic().setName("被水晶彈幕擊中").setIds({66790}).setBoss(&boss_the_dragonvoid),
		Mechanic().setName("被克拉卡托的烙印光束擊中").setIds({65017}).setBoss(&boss_the_dragonvoid),
		Mechanic().setName("被墨德摩斯的衝擊波擊中").setIds({64810}).setBoss(&boss_the_dragonvoid),
		Mechanic().setName("被澤坦的尖叫擊中").setIds({66658}).setBoss(&boss_the_dragonvoid),
		Mechanic().setName("被漩渦擊中").setIds({65252}).setBoss(&boss_the_dragonvoid),
		Mechanic().setName("被淑雯的海嘯擊中").setIds({64748, 66489}).setBoss(&boss_the_dragonvoid),
		Mechanic().setName("被淑雯的爪擊打中").setIds({63588}).setBoss(&boss_the_dragonvoid),
		Mechanic().setName("被暴露").setFailIfHit(false).setIds({890}).setSpecialRequirement(requirementOnSelfRevealedInHarvestTemple).setBoss(&boss_the_dragonvoid),
	};
	return *mechanics;
}
