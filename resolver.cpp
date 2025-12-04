#include "globals.hpp"
#include "resolver.hpp"
#include "animations.hpp"
#include "server_bones.hpp"
#include "ragebot.hpp"
#include "penetration.hpp"

namespace resolver
{
	inline void prepare_jitter(c_cs_player* player, resolver_info_t& resolver_info, anim_record_t* current)
	{
		auto& jitter = resolver_info.jitter;
		jitter.yaw_cache[jitter.yaw_cache_offset % YAW_CACHE_SIZE] = current->eye_angles.y;

		if (jitter.yaw_cache_offset >= YAW_CACHE_SIZE + 1)
			jitter.yaw_cache_offset = 0;
		else
			jitter.yaw_cache_offset++;

		for (int i = 0; i < YAW_CACHE_SIZE - 1; ++i)
		{
			float diff = std::fabsf(jitter.yaw_cache[i] - jitter.yaw_cache[i + 1]);
			if (diff <= 0.f)
			{
				if (jitter.static_ticks < 3)
					jitter.static_ticks++;
				else
					jitter.jitter_ticks = 0;
			}
			else if (diff >= 10.f)
			{
				if (jitter.jitter_ticks < 3)
					jitter.jitter_ticks++;
				else
					jitter.static_ticks = 0;
			}
		}

		jitter.is_jitter = jitter.jitter_ticks > jitter.static_ticks;
	}

#ifdef LEGACY
	int lby_update(c_cs_player* player, anim_record_t* record, resolver_info_t* data)
	{
		int return_value = 0;

		const float lby = math::normalize_yaw(player->lower_body_yaw());
		const auto hit_miss_data = RAGEBOT->missed_shots[player->index()];

		if (lby != data->lby.last_lby_value) {
			return_value = 1;

			data->lby.next_lby_update = player->sim_time() + 1.1f;
			data->lby.last_lby_value = lby;
		}

		if (player->sim_time() > data->lby.next_lby_update) {
			return_value = 1;
			data->lby.next_lby_update = player->sim_time() + 1.1f;
		}

		if (hit_miss_data > 2)
			return_value = 0;

		return return_value;
	}
#endif

	inline vec3_t get_point_direction(c_cs_player* player)
	{
		vec3_t fw{}, at_target_fw{};

		math::vector_angles(vec3_t(HACKS->local->origin() - player->get_abs_origin()), at_target_fw);
		math::angle_vectors(vec3_t(0, at_target_fw.y - 90.f, 0), fw);

		return fw;
	}

	inline void prepare_freestanding(c_cs_player* player)
	{
		auto& info = resolver_info[player->index()];
		auto& freestanding = info.freestanding;

		auto layers = player->animlayers();

		if (!layers || !HACKS->weapon_info || !HACKS->local || !HACKS->local->is_alive() || player->is_bot() || !g_cfg.rage.resolver)
		{
			if (freestanding.updated)
				freestanding.reset();

			return;
		}

		auto weight = layers[6].weight;
		if (weight > 0.75f)
		{
			if (freestanding.updated)
				freestanding.reset();

			return;
		}

		auto& cache = player->bone_cache();
		if (!cache.count() || !cache.base())
			return;

		vec3_t at_target_fw{};

		math::vector_angles(vec3_t(HACKS->local->origin() - player->get_abs_origin()), at_target_fw);

		float at_target = math::normalize_yaw(at_target_fw.y);
		float angle = math::normalize_yaw(player->eye_angles().y);

		const bool sideways_left = std::abs(math::normalize_yaw(angle - math::normalize_yaw(at_target - 90.f))) < 45.f;
		const bool sideways_right = std::abs(math::normalize_yaw(angle - math::normalize_yaw(at_target + 90.f))) < 45.f;

		bool forward = std::abs(math::normalize_yaw(angle - math::normalize_yaw(at_target + 180.f))) < 45.f;
		bool inverse_side = forward && !(sideways_left || sideways_right);

		auto direction = get_point_direction(player);

		static matrix3x4_t predicted_matrix[128]{};
		std::memcpy(predicted_matrix, cache.base(), sizeof(predicted_matrix));

		auto store_changed_matrix_data = [&](const vec3_t& new_position, bullet_t& out)
			{
				auto old_abs_origin = player->get_abs_origin();

				math::change_bones_position(predicted_matrix, 128, player->origin(), new_position);
				{
					static matrix3x4_t old_cache[128]{};
					player->store_bone_cache(old_cache);
					{
						player->set_abs_origin(new_position);
						player->set_bone_cache(predicted_matrix);

						auto head_pos = cache.base()[8].get_origin();
						out = penetration::simulate(HACKS->local, player, ANIMFIX->get_local_anims()->eye_pos, head_pos);
					}
					player->set_bone_cache(old_cache);
				}
				math::change_bones_position(predicted_matrix, 128, new_position, player->origin());
			};

		bullet_t left{}, right{};

		auto left_dir = inverse_side ? (player->origin() + direction * 40.f) : (player->origin() - direction * 40.f);
		store_changed_matrix_data(left_dir, left);

		auto right_dir = inverse_side ? (player->origin() - direction * 40.f) : (player->origin() + direction * 40.f);
		store_changed_matrix_data(right_dir, right);

		if (left.damage > right.damage)
			freestanding.side = 1;
		else if (left.damage < right.damage)
			freestanding.side = -1;

		if (freestanding.side)
			freestanding.updated = true;
	}

	inline void prepare_side(c_cs_player* player, anim_record_t* current, anim_record_t* last)
	{
		auto& info = resolver_info[player->index()];
		if (!HACKS->weapon_info || !HACKS->local || !HACKS->local->is_alive() || player->is_bot() || !g_cfg.rage.resolver)
		{
			if (info.resolved)
				info.reset();

			return;
		}

		auto state = player->animstate();
		if (!state)
		{
			if (info.resolved)
				info.reset();

			return;
		}

		auto hdr = player->get_studio_hdr();
		if (!hdr)
			return;

		if (current->choke < 2)
			info.add_legit_ticks();
		else
			info.add_fake_ticks();

		if (info.is_legit())
		{
			info.resolved = false;
			info.mode = XOR("no fake");
			return;
		}

#ifdef LEGACY
		info.resolved = true;
		info.lby_update = lby_update(player, current, &info);

		auto at_target = math::calc_angle(HACKS->local->origin(), player->origin()).normalized_angle().y;
		auto layers = player->animlayers();
		auto move = &layers[ANIMATION_LAYER_MOVEMENT_MOVE];
		auto lean = &layers[ANIMATION_LAYER_LEAN];
		auto adjust = &layers[ANIMATION_LAYER_ADJUST];

		if (player->flags().has(FL_ONGROUND))
		{
			if (player->velocity().length_2d() > 0.1f && !current->fakewalking)
			{
				info.jitter.yaw_cache[0] = player->lower_body_yaw();
				info.move.lby = player->lower_body_yaw();
				info.move.time = player->sim_time();
				info.lby.logged_lby_delta_score = 0;
				info.lby.lby_breaker_failed = false;

				info.record = *current;
				info.mode = XOR("moving");
			}
			else
			{
				if (info.record.sim_time > 0.f) {
					vec3_t delta = info.record.origin - current->origin;
					if (delta.length() <= 128.f) {
						info.lby.lby_breaker_failed = true;
					}
				}

				if (info.lby.lby_breaker_failed)
				{
					if (info.lby_update == 1 || (player->sim_time() - info.move.time) < 0.22f)
					{
						info.jitter.yaw_cache[0] = player->lower_body_yaw();
						info.mode = XOR("flick");
						return;
					}
				}

				auto& misses = RAGEBOT->missed_shots[player->index()];

				if (misses > 0)
				{
					info.jitter.yaw_cache[0] = (at_target + 180.f) + (90.f * (misses % 3));
					info.mode = XOR("brute");
				}
				else
				{
					info.jitter.yaw_cache[0] = info.move.lby;
					info.mode = XOR("last move lby");
				}
			}
		}
		else
		{
			info.jitter.yaw_cache[0] = at_target;
			info.move.lby = 0.f;
			info.move.time = 0.f;
			info.lby.logged_lby_delta_score = 0;
			info.lby.lby_breaker_failed = false;
			info.record.reset();
			info.mode = XOR("air");
		}
#else
		prepare_jitter(player, info, current);
		auto& jitter = info.jitter;
		prepare_freestanding(player);
		auto& freestanding = info.freestanding;

		if (jitter.is_jitter)
		{
			auto& misses = RAGEBOT->missed_shots[player->index()];
			if (misses > 0)
				info.side = 1337;
			else
			{
				float first_angle = math::normalize_yaw(jitter.yaw_cache[YAW_CACHE_SIZE - 1]);
				float second_angle = math::normalize_yaw(jitter.yaw_cache[YAW_CACHE_SIZE - 2]);

				float _first_angle = std::sin(DEG2RAD(first_angle));
				float _second_angle = std::sin(DEG2RAD(second_angle));

				float __first_angle = std::cos(DEG2RAD(first_angle));
				float __second_angle = std::cos(DEG2RAD(second_angle));

				float avg_yaw = math::normalize_yaw(RAD2DEG(std::atan2f((_first_angle + _second_angle) / 2.f, (__first_angle + __second_angle) / 2.f)));
				float diff = math::normalize_yaw(current->eye_angles.y - avg_yaw);

				info.side = diff > 0.f ? -1 : 1;
			}

			info.resolved = true;
			info.mode = XOR("jitter");
		}
		else
		{
			auto& misses = RAGEBOT->missed_shots[player->index()];

			if (freestanding.updated)
			{
				info.side = freestanding.side;
				info.mode = XOR("freestanding");
			}
			else if (misses > 0)
			{
				switch (misses % 3)
				{
				case 1:
					info.side = -1;
					break;
				case 2:
					info.side = 1;
					break;
				case 0:
					info.side = 0;
					break;
				}

				info.resolved = true;
				info.mode = XOR("brute");
			}
			else
			{
				info.side = 0;
				info.mode = XOR("static");

				info.resolved = true;
			}
		}
#endif
	}

	inline void apply_side(c_cs_player* player, anim_record_t* current, int choke)
	{
		auto& info = resolver_info[player->index()];
		if (!HACKS->weapon_info || !HACKS->local || !HACKS->local->is_alive() || !info.resolved || info.side == 1337 || player->is_teammate(false))
			return;

		auto state = player->animstate();
		if (!state)
			return;

#ifdef LEGACY
		current->eye_angles.y = math::normalize_yaw(info.jitter.yaw_cache[0]);
#else
		float desync_angle = choke < 2 ? state->get_max_rotation() : 120.f;
		state->abs_yaw = math::normalize_yaw(player->eye_angles().y + desync_angle * info.side);
#endif
	}
}