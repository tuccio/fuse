#pragma once

#include <DirectXMath.h>

struct cb_camera
{

	DirectX::XMFLOAT3 position;

	float             fovy;
	float             aspectRatio;

	float             znear;
	float             zfar;

	float             __fill;

	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
	DirectX::XMMATRIX viewProjection;
	DirectX::XMMATRIX invViewProjection;

};

struct cb_material
{

	DirectX::XMFLOAT3 baseColor;

	float             metallic;
	float             roughness;
	float             specular;

	float             __fill[2];

};

struct cb_transform
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldView;
	DirectX::XMMATRIX worldViewProjection;
};

struct cb_light
{

	DirectX::XMFLOAT3 luminance;
	DirectX::XMFLOAT3 ambient;

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 direction;

	float             spotAngle;

	float             __fill[3];

};

struct cb_per_frame
{
	cb_camera camera;
};

struct cb_per_object
{
	cb_transform transform;
	cb_material  material;
};

struct cb_per_light
{
	cb_light light;
};