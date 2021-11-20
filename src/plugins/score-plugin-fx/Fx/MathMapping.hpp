#pragma once
#include <Fx/MathGenerator.hpp>
namespace Nodes
{
namespace MathMapping
{
struct Node
{
  struct Metadata : Control::Meta_base
  {
    static const constexpr auto prettyName = "Custom Mapping (Value)";
    static const constexpr auto objectKey = "MathMapping";
    static const constexpr auto category = "Control";
    static const constexpr auto author = "ossia score, ExprTK (Arash Partow)";
    static const constexpr auto kind = Process::ProcessCategory::Mapping;
    static const constexpr auto description
        = "Applies a math expression to an input.\n"
          "Available variables: a,b,c, t (samples), dt (delta), pos (position "
          "in parent), x (value)\n"
          "See the documentation at http://www.partow.net/programming/exprtk";
    static const constexpr auto tags = std::array<const char*, 0>{};
    static const uuid_constexpr auto uuid
        = make_uuid("ae84e8b6-74ff-4259-aeeb-305d95cdfcab");

    static const constexpr value_in value_ins[]{value_in{"in", false}};
    static const constexpr value_out value_outs[]{"out"};

    static const constexpr auto controls = tuplet::make_tuple(
        Control::LineEdit(
            "Expression (ExprTK)",
            "cos(t) + log(pos * (1+abs(x)) / dt)"),
        Control::FloatSlider("Param (a)", 0., 1., 0.5),
        Control::FloatSlider("Param (b)", 0., 1., 0.5),
        Control::FloatSlider("Param (c)", 0., 1., 0.5));
  };
  struct State
  {
    State()
    {
      expr.add_variable("x", cur_value);
      expr.add_variable("t", cur_time);
      expr.add_variable("dt", cur_deltatime);
      expr.add_variable("pos", cur_pos);
      expr.add_variable("a", p1);
      expr.add_variable("b", p2);
      expr.add_variable("c", p3);
      expr.add_variable("m1", m1);
      expr.add_variable("m2", m2);
      expr.add_variable("m3", m3);
      expr.add_constants();

      expr.register_symbol_table();
    }
    double cur_value{};
    double cur_time{};
    double cur_deltatime{};
    double cur_pos{};
    double p1{}, p2{}, p3{};
    double m1{}, m2{}, m3{};
    ossia::math_expression expr;
    int64_t last_value_time{};

    bool ok = false;
  };

  using control_policy = ossia::safe_nodes::last_tick;
  static void
  run(const ossia::value_port& input,
      const std::string& expr,
      float a,
      float b,
      float c,
      ossia::value_port& output,
      ossia::token_request tk,
      ossia::exec_state_facade st,
      State& self)
  {
    if (!self.expr.set_expression(expr))
      return;

    auto ratio = st.modelToSamples();
    auto parent_dur = tk.parent_duration.impl * ratio;
    for (const ossia::timed_value& v : input.get_data())
    {
      int64_t new_time = tk.prev_date.impl * ratio + v.timestamp;
      setMathExpressionTiming(self, new_time, self.last_value_time, parent_dur);
      self.last_value_time = new_time;

      self.cur_value = ossia::convert<double>(v.value);
      self.p1 = a;
      self.p2 = b;
      self.p3 = c;

      auto res = self.expr.value();
      output.write_value(res, v.timestamp);
    }
  }

  template <typename... Args>
  static void item(Args&&... args)
  {
    Nodes::mathItem(Metadata::controls, std::forward<Args>(args)...);
  }
};
}

namespace MathAudioFilter
{
struct Node
{
  struct Metadata : Control::Meta_base
  {
    static const constexpr auto prettyName = "Audio Filter";
    static const constexpr auto objectKey = "MathAudioFilter";
    static const constexpr auto category = "Audio";
    static const constexpr auto author = "ossia score, ExprTK (Arash Partow)";
    static const constexpr auto tags = std::array<const char*, 0>{};
    static const constexpr auto kind = Process::ProcessCategory::AudioEffect;
    static const constexpr auto description
        = "Applies a math expression to an audio input.\n"
          "Available variables: a,b,c, t (samples), fs (sampling frequency), "
          "\n"
          "x (value), px (previous value)\n"
          "See the documentation at http://www.partow.net/programming/exprtk";
    static const uuid_constexpr auto uuid
        = make_uuid("13e1f4b0-1c2c-40e6-93ad-dfc91aac5335");

    static const constexpr audio_in audio_ins[]{"in"};
    static const constexpr audio_out audio_outs[]{"out"};

    static const constexpr auto controls = tuplet::make_tuple(
        Control::LineEdit(
            "Expression (ExprTK)",
            "var n := x[];\n"
            "\n"
            "for (var i := 0; i < n; i += 1) {\n"
            "  var dist := tan(x[i]*log(1 + 200 * a));\n"
            "  out[i] := clamp(0, dist, 1);\n"
            "}\n"),
        Control::FloatSlider("Param (a)", 0., 1., 0.5),
        Control::FloatSlider("Param (b)", 0., 1., 0.5),
        Control::FloatSlider("Param (c)", 0., 1., 0.5));
  };

  struct State
  {
    State()
    {
      cur_in.reserve(8);
      cur_out.reserve(8);
      prev_in.reserve(8);
      m1.reserve(8);
      m2.reserve(8);
      m3.reserve(8);
      cur_in.resize(2);
      cur_out.resize(2);
      prev_in.resize(2);
      m1.resize(2);
      m2.resize(2);
      m3.resize(2);

      expr.add_vector("x", cur_in);
      expr.add_vector("out", cur_out);
      expr.add_vector("px", prev_in);
      expr.add_variable("t", cur_time);
      expr.add_variable("a", p1);
      expr.add_variable("b", p2);
      expr.add_variable("c", p3);
      expr.add_vector("m1", m1);
      expr.add_vector("m2", m2);
      expr.add_vector("m3", m3);
      expr.add_constant("fs", fs);
      expr.add_constants();

      expr.register_symbol_table();
    }

    void reset_symbols(std::size_t N)
    {
      if (N == cur_in.size())
        return;

      expr.remove_vector("x");
      expr.remove_vector("out");
      expr.remove_vector("px");
      expr.remove_vector("m1");
      expr.remove_vector("m2");
      expr.remove_vector("m3");

      cur_in.resize(N);
      cur_out.resize(N);
      prev_in.resize(N);
      m1.resize(N);
      m2.resize(N);
      m3.resize(N);

      expr.add_vector("x", cur_in);
      expr.add_vector("out", cur_out);
      expr.add_vector("px", prev_in);
      expr.add_vector("m1", m1);
      expr.add_vector("m2", m2);
      expr.add_vector("m3", m3);
    }

    std::vector<double> cur_in{};
    std::vector<double> cur_out{};
    std::vector<double> prev_in{};
    double cur_time{};
    double p1{}, p2{}, p3{};
    std::vector<double> m1, m2, m3;
    double fs{44100};
    ossia::math_expression expr;
    bool ok = false;
  };

  using control_policy = ossia::safe_nodes::last_tick;
  static void
  run(const ossia::audio_port& input,
      const std::string& expr,
      float a,
      float b,
      float c,
      ossia::audio_port& output,
      ossia::token_request tk,
      ossia::exec_state_facade st,
      State& self)
  {
    if (tk.date > tk.prev_date)
    {
      if (!self.expr.set_expression(expr))
        return;

      const auto samplesRatio = st.modelToSamples();
      const auto [tick_start, count] = st.timings(tk);

      if (input.empty())
        return;

      const auto min_count
          = std::min((int64_t)input.channel(0).size() - tick_start, count);

      const int chans = input.channels();
      self.reset_symbols(chans);
      output.set_channels(chans);

      for (int j = 0; j < chans; j++)
      {
        auto& out = output.channel(j);
        out.resize(st.bufferSize());
      }

      self.p1 = a;
      self.p2 = b;
      self.p3 = c;
      self.fs = st.sampleRate();
      const auto start_sample = (tk.prev_date * samplesRatio).impl;
      for (int64_t i = 0; i < min_count; i++)
      {
        for (int j = 0; j < chans; j++)
        {
          self.cur_in[j] = input.channel(j)[tick_start + i];
        }
        self.cur_time = start_sample + i;

        // Compute the value
        self.expr.value();

        // Apply the output
        for (int j = 0; j < chans; j++)
        {
          output.channel(j)[tick_start + i] = self.cur_out[j];
        }
        std::swap(self.cur_in, self.prev_in);
      }
    }
  }

  template <typename... Args>
  static void item(Args&&... args)
  {
    Nodes::mathItem(Metadata::controls, std::forward<Args>(args)...);
  }
};
}


namespace MicroMapping
{
struct Node
{
  struct Metadata : Control::Meta_base
  {
    static const constexpr auto prettyName = "Micromap";
    static const constexpr auto objectKey = "MicroMapping";
    static const constexpr auto category = "Control";
    static const constexpr auto author = "ossia score, ExprTK (Arash Partow)";
    static const constexpr auto kind = Process::ProcessCategory::Mapping;
    static const constexpr auto description
        = "Applies a math expression to an input.";
    static const constexpr auto tags = std::array<const char*, 0>{};
    static const uuid_constexpr auto uuid
        = make_uuid("25c64b87-a44a-4fed-9f60-0a48906fd3ec");

    static const constexpr value_in value_ins[]{value_in{"in", false}};
    static const constexpr value_out value_outs[]{"out"};

    static const constexpr auto controls = tuplet::make_tuple(
        Control::LineEdit("Expression", "x / 127"));
  };
  struct State
  {
    State()
    {
      expr.add_variable("x", cur_value);
      expr.add_variable("px", prev_value);
      expr.add_variable("t", cur_time);
      expr.add_variable("dt", cur_deltatime);
      expr.add_variable("pos", cur_pos);
      expr.add_constants();

      expr.register_symbol_table();
    }
    double cur_value{};
    double prev_value{};
    double cur_time{};
    double cur_deltatime{};
    double cur_pos{};
    ossia::math_expression expr;
    int64_t last_value_time{};

    bool ok = false;
  };

  using control_policy = ossia::safe_nodes::last_tick;
  static void
  run(const ossia::value_port& input,
      const std::string& expr,
      ossia::value_port& output,
      const ossia::token_request& tk,
      ossia::exec_state_facade st,
      State& self)
  {
    if (!self.expr.set_expression(expr))
      return;

    auto ratio = st.modelToSamples();
    auto parent_dur = tk.parent_duration.impl * ratio;
    for (const ossia::timed_value& v : input.get_data())
    {
      int64_t new_time = tk.prev_date.impl * ratio + v.timestamp;
      setMathExpressionTiming(self, new_time, self.last_value_time, parent_dur);
      self.last_value_time = new_time;

      self.cur_value = ossia::convert<double>(v.value);

      auto res = self.expr.value();

      self.prev_value = self.cur_value;

      output.write_value(res, v.timestamp);
    }
  }

  template <typename... Args>
  static void item(Args&&... args)
  {
    Nodes::miniMathItem(Metadata::controls, std::forward<Args>(args)...);
  }
};
}
}

namespace Control
{
template <>
struct HasCustomUI<Nodes::MathAudioFilter::Node> : std::true_type
{
};
template <>
struct HasCustomUI<Nodes::MathAudioGenerator::Node> : std::true_type
{
};
template <>
struct HasCustomUI<Nodes::MathMapping::Node> : std::true_type
{
};
template <>
struct HasCustomUI<Nodes::MicroMapping::Node> : std::true_type
{
};
template <>
struct HasCustomUI<Nodes::MathGenerator::Node> : std::true_type
{
};
}
