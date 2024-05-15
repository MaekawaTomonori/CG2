#pragma once
class Object{
    // ==Objectに必要なもの==
    // VertexResource
    // VertexBufferView (VBV)
    // TransformMatrix用のCBV
    // CPUで扱うTransform
private:
ID3D12Device* device_;

protected:
	ID3D12Resource* vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
    Transform transform_;
};

