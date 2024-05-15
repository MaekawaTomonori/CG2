#pragma once
class DataContainer final{
public:
	DataContainer(const DataContainer&) = delete;
	DataContainer(const DataContainer&&) = delete;
private:
	DataContainer() = default;
    ~DataContainer() = default;

    //Members
    IDXGIFactory7* dxgiFactory_ = nullptr;
    IDXGIAdapter4* useAdapter_ = nullptr;
    ID3D12Device* device_ = nullptr;

public:
    DataContainer& operator=(const DataContainer&) = delete;
    DataContainer& operator=(const DataContainer&&) = delete;

    static DataContainer& getInstance();

    ID3D12Device* getDevice() const {
    	return device_;
    }
    IDXGIFactory7* getDXGIFactory()const {
        return dxgiFactory_;
    }

    void RegisteringDevice();
    void Destroy() const;
};