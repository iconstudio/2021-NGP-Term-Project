#pragma once


class GameSprite {
public:
	GameSprite(HINSTANCE instance, UINT resource, UINT number = 1, int xoff = 0, int yoff = 0);
	GameSprite(LPCTSTR path, UINT number = 1, int xoff = 0, int yoff = 0);
	~GameSprite();

	void draw(HDC surface, double x, double y, double index = 0.0, double angle = 0.0, double xscale = 1.0, double yscale = 1.0, double alpha = 1.0);

	void set_bbox(const LONG left, const LONG right, const LONG top, const LONG bottom);

	const int get_width() const;
	const int get_height() const;

	const u_int number; // �̹��� ���
	const int xoffset, yoffset; // ��������Ʈ�� �߽���
	RECT bbox;

private:
	bool __process_image(CImage&, const size_t = 0, const size_t = 0);
	void __draw_single(HDC, CImage&, double, double, double = 0.0, double = 1.0, double = 1.0, double = 1.0);

	CImage raw; // ���� �׸�.
	int raw_width, raw_height; // ���� ũ��

	vector<unique_ptr<CImage>> frames; // �߸� �׸�. ������� ���� �ִ�.
	int width, height; // ������ ũ��
};

shared_ptr<GameSprite> make_sprite(HINSTANCE instance, UINT resource, UINT number = 1, int xoff = 0, int yoff = 0);

shared_ptr<GameSprite> make_sprite(LPCTSTR path, UINT number = 1, int xoff = 0, int yoff = 0);

