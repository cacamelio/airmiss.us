#include "globals.hpp"
#include "clantag.hpp"

const char8_t* icons[]
{
	u8"\u2621",
	u8"\u2620",
	u8"\u2623",
	u8"\u262F",
	u8"\u267B",
	u8"\u26A1",
	u8"\u26A3",
};

void  c_clantag::run()
{

	static auto tag_desc = XOR(" airflow ");
	static bool reset_tag = true;
	static int last_clantag_time = 0;
	static float next_update_time = 0.f;

	float predicted_curtime = TICKS_TO_TIME(HACKS->local->tickbase());
	int clantag_time = (int)(HACKS->global_vars->curtime * 2.f) + HACKS->ping;

	if (g_cfg.misc.clantag)
	{
		reset_tag = false;

		if (clantag_time != last_clantag_time)
		{
			if (next_update_time <= predicted_curtime || next_update_time - predicted_curtime > 1.f)
			{
				static std::string tag_text_2 = XOR(" airflow ");

				std::string tag_start = (const char*)icons[int(HACKS->global_vars->curtime * 2.4f) % ARRAYSIZE(icons)];

				std::string tag_text = tag_start + tag_text_2 + tag_start;
				set_clan_tag(tag_text.c_str(), tag_desc.c_str());
			}

			last_clantag_time = clantag_time;
		}
	}
	else
	{
		if (!reset_tag)
		{
			if (clantag_time != last_clantag_time)
			{
				set_clan_tag("", "");

				reset_tag = true;
				last_clantag_time = clantag_time;
			}
		}
	}
}