#include "pch.h"
#include "stdafx.h"
#include "Sprite.h"


GameSprite::GameSprite(HINSTANCE instance, UINT resource, UINT number, int xoff, int yoff)
	: raw(CImage()), raw_width(0), raw_height(0), width(0), height(0), number(number), xoffset(xoff), yoffset(yoff) {
	raw.LoadFromResource(instance, resource);
	if (raw.IsNull()) {
		WCHAR temp[256];

		swprintf_s(temp, TEXT("�ڿ� ��������Ʈ %u��(��) �ҷ����� �߿� ������ �߻��߽��ϴ�."), resource);

		int error = MessageBox(NULL, temp, TEXT("����"), MB_OK);

		if (error) {
			SendMessage(NULL, WM_CLOSE, 0, 0);
		}
	}

	raw.SetHasAlphaChannel(true);
	raw_width = raw.GetWidth();
	raw_height = raw.GetHeight();

	auto result = __process_image(raw, raw_width, raw_height);
	if (!result) {
		WCHAR temp[256];

		swprintf_s(temp, TEXT("�ڿ� ��������Ʈ %u��(��) �ùٸ� ũ�⸦ ���� ���� �ʽ��ϴ�."), resource);

		int error = MessageBox(NULL, temp, L"����", MB_OK);
		if (error) {
			SendMessage(NULL, WM_CLOSE, 0, 0);
		}
	}
}

GameSprite::GameSprite(LPCTSTR path, UINT number, int xoff, int yoff)
	: raw(CImage()), raw_width(0), raw_height(0), width(0), height(0), number(number), xoffset(xoff), yoffset(yoff) {
	raw.Load(path);
	if (raw.IsNull()) {
		WCHAR temp[256];

		swprintf_s(temp, TEXT("��� %s���� ��������Ʈ�� �ҷ��� �� �����ϴ�."), path);

		int error = MessageBox(NULL, temp, TEXT("����"), MB_OK);

		if (error) {
			SendMessage(NULL, WM_CLOSE, 0, 0);
		}
	}

	raw.SetHasAlphaChannel(true);
	raw_width = raw.GetWidth();
	raw_height = raw.GetHeight();

	auto result = __process_image(raw, raw_width, raw_height);
	if (!result) {
		WCHAR temp[256];

		swprintf_s(temp, TEXT("%s�� ��ġ�� �׸� ������ �ùٸ� ũ�⸦ ���� ���� �ʽ��ϴ�."), path);

		int error = MessageBox(NULL, temp, TEXT("����"), MB_OK);
		if (error) {
			SendMessage(NULL, WM_CLOSE, 0, 0);
		}
	}
}

GameSprite::~GameSprite() {
	if (0 < number) {
		for (auto& image : frames) {
			image->Destroy();
		}
	}
	raw.Destroy();

	frames.clear();
}

void GameSprite::draw(HDC surface, double x, double y, double index, double angle, double xscale, double yscale, double alpha) {
	if (1 < number) {
		auto frame = frames.at((u_int)index).get();
		__draw_single(surface, *frame, x, y, angle, xscale, yscale, alpha);
	} else {
		__draw_single(surface, raw, x, y, angle, xscale, yscale, alpha);
	}
}

void GameSprite::set_bbox(LONG left, LONG right, LONG top, LONG bottom) {
	bbox.left = left;
	bbox.right = right;
	bbox.top = top;
	bbox.bottom = bottom;
}

const int GameSprite::get_width() const {
	return width;
}

const int GameSprite::get_height() const {
	return height;
}

bool GameSprite::__process_image(CImage& image, size_t width, size_t height) {
	if (0 < width && 0 < height) {
		if (1 < number) { // �ִϸ��̼��� ���ؼ��� ���η� ������ �׸��� �ʿ��մϴ�.
			int slice_w, temp_w;
			if (1 == width) {
				slice_w = 1;
			} else {
				temp_w = (width / number);
				if (temp_w < 2) { // 0, 1
					slice_w = 1;
				} else {
					slice_w = temp_w;
				}
			}

			// ����
			frames.reserve(number);

			auto raw_bitlevel = raw.GetBPP();

			for (UINT i = 0; i < number; ++i) { // ������ ����
				// 1. ������ �׸� ����
				auto image_slice = new CImage(); // make_shared<CImage>();
				image_slice->Create(slice_w, height, raw_bitlevel);
				image_slice->SetHasAlphaChannel(true);

				auto slice_buffer = image_slice->GetDC();

				// 2. ���� �׸��� (i * slice_width, 0)�� ��ġ�� ������ ���� �׸��� (0, 0) ��ġ�� ����
				raw.BitBlt(slice_buffer, 0, 0, slice_w, height, i * slice_w, 0, SRCCOPY);

				// 3. �޸� ����ȭ
				image_slice->ReleaseDC(); // slice_buffer ����

				// 4. ���� ���� (������ �������� ���� ���� �Ұ�)
				frames.emplace_back(image_slice);
			}

			this->width = slice_w;
			this->height = raw_height;
		} else { // one frame
			this->width = raw_width;
			this->height = raw_height;
		}

		set_bbox(-xoffset, this->width - xoffset, -yoffset, this->height - yoffset);
	} else { // failed!
		return false;
	}

	return true;
}

void GameSprite::__draw_single(HDC surface, CImage& image, double dx, double dy, double angle, const double xscale, double yscale, double alpha) {
	int nGraphicsMode = SetGraphicsMode(surface, GM_ADVANCED);
	float cosine = (float)lengthdir_x(1, angle);
	float sine = (float)lengthdir_y(1, angle);

	// �����ʹ� �޸� y ��ǥ�� ������
	XFORM xform;
	if (0.0 != angle) {
		xform.eM11 = cosine * xscale;
		xform.eM12 = sine;
		xform.eM21 = -sine;
		xform.eM22 = cosine * yscale;
	} else {
		xform.eM11 = xscale;
		xform.eM12 = 0.0f;
		xform.eM21 = 0.0f;
		xform.eM22 = yscale;
	}
	xform.eDx = floor(dx - xoffset * xscale);
	xform.eDy = floor(dy - yoffset * yscale);

	Render::transform_set(surface, xform);

	if (alpha != 1.0)
		image.AlphaBlend(surface, 0, 0, width * abs(xscale), height * abs(yscale), 0, 0, width, height, (BYTE)(255 * alpha));
	else
		image.Draw(surface, 0, 0, width * abs(xscale), height * abs(yscale), 0, 0, width, height);

	Render::transform_set_identity(surface);
	SetGraphicsMode(surface, nGraphicsMode);
}

shared_ptr<GameSprite> make_sprite(HINSTANCE instance, UINT resource, UINT number, int xoff, int yoff) {
	return make_shared<GameSprite>(instance, resource, number, xoff, yoff);
}

shared_ptr<GameSprite> make_sprite(LPCTSTR path, UINT number, int xoff, int yoff) {
	return make_shared<GameSprite>(path, number, xoff, yoff);
}
