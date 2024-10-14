#pragma once
#include "Object/Character.h"


class Boss final:
    public Character{
public:
    Boss();
    ~Boss()override;

    void Initialize(std::vector<Object3d*>models);

    void Update()override;

    void Draw()override;

private:
    void Move();

private:

    float moveSpeed_ = 0.01f;
};

