#pragma once

#include <optional>
#include <functional>
#include <string_view>
#include <diva.h>

enum KeyAction : int32_t
{
	KeyAction_None      = 0,
	KeyAction_MoveUp    = 1,
	KeyAction_MoveDown  = 2,
	KeyAction_MoveLeft  = 3,
	KeyAction_MoveRight = 4,
	KeyAction_Enter     = 5,
	KeyAction_Cancel    = 6,
	KeyAction_SwapLeft  = 7,
	KeyAction_SwapRight = 8,
	KeyAction_Preview   = 9
};

class AetElement
{
public:
	AetElement() = default;
	AetElement(uint32_t scene_id) : scene_id(scene_id) { }

	AetElement(AetElement&& other) noexcept
	{
		scene_id = other.scene_id;
		handle = other.handle;
		args = other.args;
		other.handle = 0;
	}

	~AetElement() { DeleteHandle(); }

	virtual void Ctrl();
	virtual void Disp();

	inline void SetScene(uint32_t id) { scene_id = id; }
	bool SetLayer(std::string name, int32_t flags, int32_t prio, int32_t res_mode, std::string_view start_marker, std::string_view end_marker, const diva::vec3* pos);
	bool SetLayer(std::string name, int32_t flags, int32_t prio, int32_t res_mode, int32_t action);

	std::optional<AetLayout> GetLayout(std::string layer_name) const;

	void SetPosition(const diva::vec3& pos);
	void SetOpacity(float opacity);
	void SetLoop(bool loop);
	void SetMarkers(const std::string& start_marker, const std::string& end_marker);

	inline const AetArgs& GetArgs() { return args; }
	inline bool Ended() const { return aet::GetEnded(handle); }

	bool DrawSpriteAt(std::string layer_name, uint32_t id, int32_t prio = 1) const;
private:
	uint32_t scene_id = 0;
	AetHandle handle = 0;
	AetArgs args = { };
	std::string layer_name;

	void DeleteHandle();
};

class AetControl : public AetElement
{
public:
	void AllowInputsWhenBlocked(bool allow);
	void SetFocus(bool focused);
	inline bool IsFocused() const { return focused; }

	virtual void OnActionPressed(int32_t action);
	virtual void OnActionPressedOrRepeat(int32_t action);

	virtual void Ctrl() override;
protected:
	bool abs_input = false;
	bool focused = true;
};

class HorizontalSelector : public AetControl
{
public:
	float font_scale = 0.85f;
	float text_opacity = 1.0f;
	bool bold_text = true;
	bool squish_text = true;

	HorizontalSelector()
	{
		SetFocus(false);
	}

	HorizontalSelector(uint32_t scene_id, std::string layer_name, int32_t prio, int32_t res_mode)
	{
		SetFocus(false);
		SetScene(scene_id);
		SetLayer(layer_name, 0x10000, prio, res_mode, "st_in", "ed_in", nullptr);
	}

	virtual void OnActionPressedOrRepeat(int32_t action) override;
	virtual void OnActionPressed(int32_t action) override;
	virtual void Ctrl() override;
	virtual void Disp() override;

	inline void SetExtraData(const void* data) { extra_data = data; }
	inline void SetPreviewNotifier(std::function<void(HorizontalSelector*, int32_t, const void*)> func) { preview_notify = func; }
protected:
	virtual std::string GetSelectedValue() = 0;
	virtual void ChangeValue(int32_t dir) = 0;

private:
	std::optional<std::function<void(HorizontalSelector*, int32_t, const void*)>> preview_notify;
	const void* extra_data = nullptr;
	bool focused_old = false;
	int32_t enter_anim_state = -1;
};

class HorizontalSelectorMulti : public HorizontalSelector
{
public:
	std::vector<std::string> values;
	int32_t selected_index = 0;

	HorizontalSelectorMulti() = default;
	HorizontalSelectorMulti(uint32_t scene_id, std::string layer_name, int32_t prio, int32_t res_mode) :
		HorizontalSelector(scene_id, layer_name, prio, res_mode) { }

	inline void SetOnChangeNotifier(std::function<void(int32_t, const std::string&)> func) { notify = func; }
protected:
	std::optional<std::function<void(int32_t, const std::string&)>> notify;

	std::string GetSelectedValue() override;
	void ChangeValue(int32_t dir) override;
};