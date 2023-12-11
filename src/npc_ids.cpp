#include "npc_ids.h"

Boss::Boss()
{
    ids = std::vector<uint32_t>();
    name = "";
    timer = 0;
    health = 0;
	pulls = 0;
}

bool Boss::hasId(uint32_t new_id)
{
    for(uint16_t index=0;index<ids.size();index++)
    {
        if(new_id == ids.at(index))
        {
            return true;
        }
    }
    return false;
}

bool Boss::operator==(Boss * other_boss)
{
	return other_boss && ids.size()>0 && other_boss->ids.size()>0 
		&& ids[0] == other_boss->ids[0];
}

Boss boss_generic = Boss().setName("Generic");

//Raids
//W1
Boss boss_vg = Boss().setIds({ 0x3C4E }).setTimer(8 * 60 * 1000).setHealth(22021440).setName("山谷守護者(Vale Guardian)");
Boss boss_gors = Boss().setIds({ 0x3C45 }).setTimer(7 * 60 * 1000).setHealth(21628200).setName("多變藤 戈瑟瓦爾(Gorseval the Multifarious)");
Boss boss_sab = Boss().setIds({ 0x3C0F }).setTimer(9 * 60 * 1000).setHealth(34015256).setName("破壞者 薩蓓莎(Sabetha the Saboteur)");
//W2
Boss boss_sloth = Boss().setIds({ 0x3EFB }).setTimer(7 * 60 * 1000).setHealth(18973828).setName("斯洛薩索(Slothasor)");
Boss boss_trio = Boss().setIds({ 0x3ED8,0x3F09,0x3EFD }).setTimer(7 * 60 * 1000).setName("強盜三人組(Bandit Trio)");//TODO: verify enrage timer
Boss boss_matti = Boss().setIds({ 0x3EF3 }).setTimer(10 * 60 * 1000).setHealth(25953840).setName("馬蒂亞斯·加布瑞爾(Matthias Gabrel)");//TODO: verify enrage timer
//W3
Boss boss_kc = Boss().setIds({ 0x3F6B }).setTimer(10 * 60 * 1000).setHealth(55053600).setName("要塞構造體(Keep Construct)");//TODO: verify enrage timer
Boss boss_xera = Boss().setIds({ 0x3F76,0x3F9E }).setTimer(11 * 60 * 1000).setHealth(22611300).setName("琦拉(Xera)");//TODO: verify enrage timer
//W4
Boss boss_cairn = Boss().setIds({ 0x432A }).setTimer(8 * 60 * 1000).setHealth(19999998).setName("不屈的凱玲(Cairn the Indomitable)");//TODO: verify enrage timer
Boss boss_mo = Boss().setIds({ 0x4314 }).setTimer(6 * 60 * 1000).setHealth(22021440).setName("末世魔監工(Mursaat Overseer)");//TODO: verify enrage timer
Boss boss_sam = Boss().setIds({ 0x4324 }).setTimer(11 * 60 * 1000).setHealth(29493000).setName("薩瑪洛格(Samarog)");//TODO: verify enrage timer
Boss boss_deimos = Boss().setIds({ 0x4302 }).setTimer(12 * 60 * 1000).setHealth(50049000).setName("戴莫斯(Deimos)");//TODO: verify enrage timer
//W5
Boss boss_sh = Boss().setIds({ 0x4D37 }).setTimer(8 * 60 * 1000).setHealth(35391600).setName("無魂懼魔(Soulless Horror)");
Boss boss_soul_eater = Boss().setIds({ 0x4C50 }).setHealth(1720425).setName("三雕像 - 食魂者(Statues - Soul Eater)");
Boss boss_ice_king = Boss().setIds({ 0x4CEB }).setTimer((3 * 60 + 30) * 1000).setHealth(9831000).setName("三雕像 - 破裂國王(Statues - Ice King)");
Boss boss_cave = Boss().setIds({ 0x4CC3,0x4D84 }).setHealth(2457750).setName("三雕像 - 洞穴(Statues - Cave)");//North: Eye of Judgement, South: Eye of Fate
Boss boss_dhuum = Boss().setIds({ 0x4BFA }).setTimer(10 * 60 * 1000).setHealth(32000000).setName("德姆(Dhuum)");
//W6
Boss boss_ca = Boss().setIds({ 43974,37464,10142 }).setTimer(8 * 60 * 1000).setName("咒術融合體(Conjured Amalgamate)");//TODO: Get boss HP
Boss boss_largos = Boss().setIds({ 21105, 21089 }).setTimer(2 * 4 * 60 * 1000).setHealth(17548336).setName("孿生蝶翼人(Twin Largos)");//using Nikare (left side hp)
Boss boss_qadim = Boss().setIds({ 20934 }).setTimer(13 * 60 * 1000).setHealth(19268760).setName("卡迪姆(Qadim)");
//W7
Boss boss_adina = Boss().setIds({ 22006 }).setTimer(8 * 60 * 1000).setHealth(22611300).setName("艾迪娜(Adina)");
Boss boss_sabir = Boss().setIds({ 21964 }).setHealth(29493000).setName("薩比爾(Sabir)");
Boss boss_qadim2 = Boss().setIds({ 22000 }).setTimer(12 * 60 * 1000).setHealth(47188800).setName("無可匹敵的卡迪姆(Qadim the Peerless)");

//FOTM
Boss boss_fotm_generic = Boss().setName("FOTM 通用機制");
//Nightmare
Boss boss_mama = Boss().setIds({ 0x427D,0x4268,0x424E }).setHealth(5200519).setName("多型態機動火炮(MAMA)");//ids: cm,normal,knight at the start of the trash on CM
Boss boss_siax = Boss().setIds({ 0x4284 }).setHealth(6138797).setName("不淨者西亞克斯(Siax)");//TODO get normal mode id
Boss boss_ensolyss = Boss().setIds({ 0x4234 }).setHealth(14059890).setName("無盡折磨者恩索利斯(Ensolyss of the Endless Torment)");//TODO get normal mode id
//Shattered Observatory
Boss boss_skorvald = Boss().setIds({ 0x44E0 }).setHealth(5551340).setName("破碎的斯科瓦德(Skorvald the Shattered)");//TODO get normal mode id
Boss boss_artsariiv = Boss().setIds({ 0x461D }).setHealth(5962266).setName("亞特薩里弗(Artsariiv)");//TODO get normal mode id
Boss boss_arkk = Boss().setIds({ 0x455F }).setHealth(9942250).setName("阿克(Arkk)");//TODO get normal mode id
//Sunqua Peak
Boss boss_ai = Boss().setIds({ 23254 }).setHealth(14905666).setName("悲傷施法者(Sorrowful Spellcaster)");

//IBS Strike Missions
Boss boss_icebrood_construct = Boss().setIds({ 22154, 22436 }).setTimer(12 * 60 * 1000).setHealth(11698890).setName("冰巢構造體(Icebrood Construct)");
Boss boss_voice_and_claw = Boss().setIds({ 22343, 22481, 22315 }).setTimer(10 * 60 * 1000).setHealth(8258040).setName("亡者之音和亡者之爪(Voice of the Fallen and Claw of the Fallen)");
Boss boss_fraenir = Boss().setIds({ 22492, 22436 }).setTimer(10 * 60 * 1000).setHealth(12387060).setName("卓瑪爪牙法蘭涅爾(Fraenir of Jormag)");
Boss boss_boneskinner = Boss().setIds({22521}).setTimer(10 * 60 * 1000).setHealth(12387060).setName("骸骨剝皮怪(Boneskinner)");
Boss boss_whisper = Boss().setIds({ 22711 }).setTimer(10 * 60 * 1000).setHealth(24774120).setName("卓瑪低語(Whisper of Jormag)");
Boss boss_coldwar = Boss().setIds({ 22836 }).setTimer(7 * 60 * 1000).setHealth(17892420).setName("風暴之聲瓦里尼亞(Varinia Stormsounder)");

//EoD Strike Missions
Boss boss_mai_trin = Boss()
                        .setName("美琴(Mai Trin)")
                        .setIds({
                            // MaiTrinStrike
                            24033,
                            // EchoOfScarletBriarNM
                            24768,
                            // EchoOfScarletBriarCM
                            25247,
                        });
Boss boss_ankka = Boss().setName("安卡(Ankka)").setIds({23957});
Boss boss_minister_li = Boss()
                            .setName("李部長(Minister Li)")
                            .setIds({
                                // MinisterLi
                                24485,
                                // MinisterLiCM
                                24266,
                                // TheSniper
                                23612,
                                // TheSniperCM
                                25259,
                                // TheMechRider
                                24660,
                                // TheMechRiderCM
                                25271,
                                // TheEnforcer
                                24261,
                                // TheEnforcerCM
                                25236,
                                // TheRitualist
                                23618,
                                // TheRitualistCM
                                25242,
                                // TheMindblade
                                24254,
                                // TheMindbladeCM
                                25280,
                            });
Boss boss_the_dragonvoid = Boss().setName("巨龍虛空(The Dragonvoid)").setIds({24375, 43488});
Boss boss_watchknight_triumvirate = Boss()
                                        .setName("守望騎士三人小組(Watchknight Triumvirate)")
                                        .setIds({
                                            // PrototypeVermilion
                                            25413,
                                            // PrototypeArsenite
                                            25415,
                                            // PrototypeIndigo
                                            25419,
                                            // PrototypeVermilionCM
                                            25414,
                                            // PrototypeArseniteCM
                                            25416,
                                            // PrototypeIndigoCM
                                            25423,
                                        });

std::list<Boss*> bosses =
{
	&boss_generic,
	&boss_vg,
	&boss_gors,
	&boss_sab,
	&boss_sloth,
	&boss_trio,
	&boss_matti,
	&boss_kc,
	&boss_xera,
	&boss_cairn,
	&boss_mo,
	&boss_sam,
	&boss_deimos,
	&boss_sh,
	&boss_soul_eater,
	&boss_ice_king,
	&boss_cave,
	&boss_dhuum,
	&boss_ca,
	&boss_largos,
	&boss_qadim,
	&boss_fotm_generic,
	&boss_mama,
	&boss_siax,
	&boss_ensolyss,
	&boss_skorvald,
	&boss_artsariiv,
	&boss_arkk,
	&boss_ai,
	&boss_icebrood_construct,
	&boss_voice_and_claw,
	&boss_fraenir,
	&boss_boneskinner,
	&boss_whisper,
	&boss_coldwar,
    &boss_mai_trin,
    &boss_ankka,
    &boss_minister_li,
    &boss_the_dragonvoid,
    &boss_watchknight_triumvirate,
};
