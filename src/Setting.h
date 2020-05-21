#pragma once
class Setting
{
public:
	struct Graphics {
		int MSAASample = 1;
		int MaxFrameInFlight = 3;
	};
private:
	Graphics graphics;
public:
	const Graphics& getGraphics() const;
};

