#pragma once

#include "../resources.hpp"
#include "../scene/scene.hpp"

#include "../main_menu.hpp"

struct BoardController {
  i32 bar_1 = 71;
  i32 bar_2 = 72;
  i32 bar_3 = 73;
  i32 bar_4 = 74;
  i32 bar_5 = 75;
  i32 bar_6 = 76;
  i32 bar_7 = 77;
  i32 bar_8 = 78;

  i32 num_active                = 0;
  f32 activation_t              = 0.f;
  const f32 ACTIVATION_DURATION = 2.f;

  i32 currently_flipping  = -1;
  f32 flip_timer          = 0;
  const f32 FLIP_DURATION = .7f;

  b8 resetting    = false;
  f32 resetting_t = 0.f;

  i32 family0_score = 0;
  i32 family1_score = 0;

  void update(f32 timestep, Scene *scene, Assets *assets)
  {
    activation_t += timestep / ACTIVATION_DURATION;
    for (i32 i = 0; i < num_active; i++) {
      Entity *bar_entity                             = scene->get(index_to_id(i));
      bar_entity->material->parameters[0].uniform_id = UniformId::RIPPLE_T;
      bar_entity->material->parameters[0].value      = fmin(activation_t - (i * 0.08), 1.f);
    }
    for (i32 i = num_active; i < 8; i++) {
      Entity *bar_entity                             = scene->get(index_to_id(i));
      bar_entity->material->parameters[0].uniform_id = UniformId::RIPPLE_T;
      bar_entity->material->parameters[0].value      = -fmin(activation_t, 1.f);
    }

    if (currently_flipping >= 0) {
      flip_timer += timestep;
      f32 rotation = 180 * (flip_timer / FLIP_DURATION);
      b8 complete  = false;
      if (rotation >= 180.f) {
        rotation = 180.f;
        complete = true;
      }
      Entity *bar               = scene->get(index_to_id(currently_flipping));
      bar->transform.rotation.x = glm::radians(rotation);

      if (complete) currently_flipping = -1;
    }

    if (resetting) {
      resetting_t += timestep;

      b8 all_reset = true;
      for (i32 i = 0; i < 8; i++) {
        Entity *bar_entity = scene->get(index_to_id(i));
        if (bar_entity->transform.rotation.x > 0.f) {
          all_reset = false;

          f32 rotation = 180 * (1 - resetting_t / FLIP_DURATION);
          if (rotation < 0.f) {
            rotation = 0.f;
          }
          bar_entity->transform.rotation.x = glm::radians(rotation);
        }
      }

      if (all_reset) {
        resetting = false;
      }
    }
  }

  void activate(Scene *scene, int n)
  {
    reset(scene);
    num_active   = n;
    activation_t = 0;
  }

  void flip(Scene *scene, Assets *assets, int index, String answer, int score)
  {
    if (index < 0 || index > 7) return;

    if (currently_flipping >= 0) {
      Entity *bar               = scene->get(index_to_id(currently_flipping));
      bar->transform.rotation.x = glm::radians(180.f);
    }

    currently_flipping = index;
    flip_timer         = 0;

    Font *font                 = assets->get_font(FONT_ANTON, 64);
    int render_target_id       = 1 + index;
    RenderTarget render_target = assets->render_targets.data[render_target_id].value;
    render_target.bind();
    render_target.clear({0, 0, 0, 0});
    const float text_scale = 2.f;
    {
      float target_border = 0.05f;
      Rect sub_target     = {0, 0, .8f * render_target.width, (float)render_target.height};
      draw_centered_text(*font, render_target, answer, sub_target, target_border, text_scale,
                         text_scale, true);
    }
    if (score > 0 && score < 100) {
      char buf[3];
      _itoa_s(score, buf, 10);

      String text;
      text.data = buf;
      text.len  = strlen(buf);

      float border    = 0.05f;
      Rect sub_target = {(1 - .18f) * render_target.width, 0, .19f * render_target.width,
                         (float)render_target.height};
      draw_centered_text(*font, render_target, text, sub_target, border, text_scale, text_scale,
                         true);
    }
    render_target.color_tex.gen_mipmaps();
  }

  void reset(Scene *scene)
  {
    resetting_t = 0;
    resetting   = true;
    num_active  = 0;
  }

  int index_to_id(int index)
  {
    int ids[] = {
        bar_1, bar_2, bar_3, bar_4, bar_5, bar_6, bar_7, bar_8,
    };
    return ids[index];
  }

  void set_score(RenderTarget target, i32 score, Assets *assets)
  {
    Font *font = assets->get_font(FONT_ANTON, 256);

    char buf[5];
    _itoa_s(score, buf, 10);

    String text;
    text.data = buf;
    text.len  = strlen(buf);

    target.bind();
    target.clear({0, 0, 0, 0});

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    draw_centered_text(*font, target, text, {0, 0, (f32)target.width, (f32)target.height}, 0.1f, 1,
                       1, true);

    target.color_tex.gen_mipmaps();

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
  }

  void set_round_score(i32 score, Assets *assets)
  {
    RenderTarget target = assets->get_render_target("board_score_round");
    set_score(target, score, assets);
  }
  void set_family0_score(i32 score, Assets *assets)
  {
    RenderTarget target = assets->get_render_target("board_score_left");
    set_score(target, score, assets);
  }
  void set_family1_score(i32 score, Assets *assets)
  {
    RenderTarget target = assets->get_render_target("board_score_right");
    set_score(target, score, assets);
  }
};

struct PlayerController {
  int player_0_0 = 53;
  int player_0_1 = 54;
  int player_0_2 = 55;
  int player_0_3 = 56;
  int player_0_4 = 57;

  int player_1_0 = 59;
  int player_1_1 = 60;
  int player_1_2 = 61;
  int player_1_3 = 62;
  int player_1_4 = 63;

  int left_faceoffer  = 91;
  int right_faceoffer = 50;

  float t = 0;

  SkeletalAnimation left_anim;
  SkeletalAnimation right_anim;
  SkeletalAnimation left_and_nod;
  Pose left, right;
  float faceoff_timer          = 0;
  const float FACEOFF_DURATION = 3.f;

  StackAllocator a;
  StackAllocator tmp;
  // TODO there should be no init function, the fanim file should be loaded in the assets file and
  // referenced
  void init()
  {
    a.init(10000000);
    tmp.init(10000000);
    left_anim =
        parse_animation(read_entire_file("resources/scenes/test/anim/anim.fanim"), {&a, &tmp});
    right_anim =
        parse_animation(read_entire_file("resources/scenes/test/anim/anim.fanim"), {&a, &tmp});
    left = left_anim.eval(0);
    right = right_anim.eval(0);
    
    left_and_nod = parse_animation(read_entire_file("resources/scenes/test/anim/left_and_nod.fanim"), {&a, &tmp});
  }

  void update(float timestep, Scene *scene, Assets *assets)
  {
    t += timestep;
    for (int i = 0; i < 5; i++) {
      int player_id  = index_to_id(0, i);
      Entity *entity = scene->get(player_id);
    }
    for (int i = 0; i < 5; i++) {
      int player_id  = index_to_id(1, i);
      Entity *entity = scene->get(player_id);
    }

    Entity *left_entity     = scene->get(left_faceoffer);
    left_entity->animation  = &left;
    Entity *right_entity    = scene->get(right_faceoffer);
    right_entity->animation = &right;
    // left_anim.update(fmod(t / FACEOFF_DURATION, 0.99));
    // right_anim.update(fmod(t / FACEOFF_DURATION, 0.99));
    left.calculate_final_mats();
    right.calculate_final_mats();
  }

  void start_faceoff() { faceoff_timer = 0; }

  int index_to_id(int family, int index)
  {
    assert(family == 0 || family == 1);
    assert(index >= 0 && index <= 4);

    int family_0[] = {
        player_0_0, player_0_1, player_0_2, player_0_3, player_0_4,
    };
    int family_1[] = {
        player_1_0, player_1_1, player_1_2, player_1_3, player_1_4,
    };
    if (family == 0) {
      return family_0[index];
    }

    return family_1[index];
  }
};

struct UiController {
  f32 banner_t;
  f32 banner_duration;
  AllocatedString<256> banner_text;

  b8 buzz_button_visible = false;
  Button2 buzz_button;
  b8 buzz_button_pressed = false;

  b8 question_visible = false;
  AllocatedString<128> question_value;
  f32 question_visible_pct = 0;

  b8 answer_textbox_visible = false;
  f32 timer_value;
  b8 timer_visible = false;
  TextBox2<32> answer_textbox;
  b8 answer_submitted = false;

  b8 pass_play_buttons_visible = false;
  Button2 play_button;
  Button2 pass_button;
  b8 play_button_pressed = false;
  b8 pass_button_pressed = false;

  void update(float timestep, RenderTarget target, InputState *input, Assets *assets)
  {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    Font *font = assets->get_font(FONT_ROBOTO_CONDENSED_REGULAR, 64);

    if (banner_t < banner_duration) {
      float base_vertical_offset = 70;
      float in_transition_offset =
          base_vertical_offset - (fmax((.25f - banner_t) * 2, 0) * base_vertical_offset);
      float out_transition_offset =
          base_vertical_offset -
          (fmax((banner_t - (banner_duration - 0.25f)) * 4, 0) * base_vertical_offset);
      float transition_offset = fmin(in_transition_offset, out_transition_offset);

      draw_text(*font, target, banner_text, 10, target.height - transition_offset, 1, 1);

      banner_t += timestep;
    }

    if (question_visible) {
      u32 visible_letters = clamp(question_visible_pct, 0, 1) * question_value.len;
      draw_text(*font, target, {question_value.data, visible_letters}, 10, 10, 1, 1);
    }

    if (timer_visible) {
      char buf[10];
      int len    = snprintf(buf, 10, "%f", timer_value);
      String val = {buf, (uint16_t)len};
      draw_text(*font, target, val, target.width - (get_text_width(*font, val) + 10), 10, 1, 1);
    }

    answer_submitted = false;
    if (answer_textbox_visible) {
      // TODO: alternative for XInput

      answer_textbox.rect = anchor_bottom_right({600, 100}, {25.f, 25.f});
      answer_textbox.update_and_draw(target, input, font);

      // TODO: allow (A) to buzz
      answer_submitted = input->key_down_events[(i32)Keys::ENTER];
    }

    buzz_button_pressed = false;
    if (buzz_button_visible) {
      buzz_button.rect = anchor_bottom_right({400, 110}, {25.f, 25.f});

      // TODO: use Space or (A) to buzz
      buzz_button.text    = "Buzz";
      buzz_button_pressed = buzz_button.update_and_draw(target, input, font);
    }

    pass_button_pressed = false;
    play_button_pressed = false;
    if (pass_play_buttons_visible) {
      pass_button.text = "PASS";
      pass_button.rect = anchor_bottom_right({400, 110}, {25.f, 25.f});

      play_button.text = "PLAY";
      play_button.rect = pass_button.rect;
      play_button.rect.x -= play_button.rect.width + 25;

      pass_button_pressed = pass_button.update_and_draw(target, input, font);
      play_button_pressed = play_button.update_and_draw(target, input, font);
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
  }

  void hide_all()
  {
    buzz_button_visible       = false;
    question_visible          = false;
    answer_textbox_visible    = false;
    timer_visible             = false;
    pass_play_buttons_visible = false;
  }

  void popup_banner(String text, f32 duration = 5.f)
  {
    banner_t        = 0;
    banner_duration = duration;
    banner_text     = string_to_allocated_string<256>(text);
  }

  void show_buzz_button(b8 visible)
  {
    pass_play_buttons_visible = false;
    answer_textbox_visible    = false;
    timer_visible             = false;

    buzz_button_visible = visible;
  }

  void answer_timer(b8 visible, f32 time = 0.f)
  {
    timer_visible = visible;
    timer_value   = time;
  }

  void show_answer_textbox(b8 visible)
  {
    pass_play_buttons_visible = false;
    buzz_button_visible       = false;

    answer_textbox_visible = visible;
  }

  void set_question(AllocatedString<128> &val)
  {
    question_value = string_to_allocated_string<128>(val);
  }
  void set_question_visible_pct(f32 p) { question_visible_pct = p; }
  void question(b8 visible) { question_visible = visible; }

  void pass_or_play(b8 visible)
  {
    buzz_button_visible    = false;
    answer_textbox_visible = false;
    timer_visible          = false;

    pass_play_buttons_visible = visible;
  }
};