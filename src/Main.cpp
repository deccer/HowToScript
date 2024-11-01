#include <mimalloc.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <ranges>
#include <span>
#include <stack>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#define POOLSTL_STD_SUPPLEMENT
#include <poolstl/poolstl.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_INCLUDE_IMPLEMENTATION
#define STB_INCLUDE_LINE_GLSL
#include <stb_include.h>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/util.hpp>

#include <debugbreak.h>
#include <spdlog/spdlog.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <entt/entt.hpp>

#ifdef USE_PROFILER
#include <tracy/TracyOpenGL.hpp>
#define PROFILER_ZONESCOPEDN(x) ZoneScopedN(x)
#else
#define PROFILER_ZONESCOPEDN(x)
#endif

#include "IconsMaterialDesign.h"
#include "IconsFontAwesome6.h"

// - Core Types ---------------------------------------------------------------

// Stolen from Jaker who stole it from https://github.com/cdgiessen/vk-module/blob/076baa98cba35cd93a6ab56c3fd1b1ea2313f806/codegen_text.py#L53
// Thanks Jaker & Charles!
#define DECLARE_FLAG_TYPE(TFlagType, TFlagTypeBits, TFlagBaseType)                                \
                                                                                                  \
struct TFlagType                                                                                  \
{                                                                                                 \
    TFlagBaseType flags = static_cast<TFlagBaseType>(0);                                          \
    constexpr TFlagType() noexcept = default;                                                     \
    constexpr explicit TFlagType(TFlagBaseType in) noexcept : flags(in) {}                        \
    constexpr TFlagType(TFlagTypeBits in) noexcept : flags(static_cast<TFlagBaseType>(in)) {}     \
    constexpr bool operator==(TFlagType const& right) const                                       \
    {                                                                                             \
        return flags == right.flags;                                                              \
    }                                                                                             \
    constexpr bool operator!=(TFlagType const& right) const                                       \
    {                                                                                             \
        return flags != right.flags;                                                              \
    }                                                                                             \
    constexpr explicit operator TFlagBaseType() const                                             \
    {                                                                                             \
        return flags;                                                                             \
    }                                                                                             \
    constexpr explicit operator bool() const noexcept                                             \
    {                                                                                             \
        return flags != 0;                                                                        \
    }                                                                                             \
};                                                                                                \
                                                                                                  \
constexpr TFlagType operator|(TFlagType a, TFlagType b) noexcept                                  \
{                                                                                                 \
    return static_cast<TFlagType>(a.flags | b.flags);                                             \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType operator&(TFlagType a, TFlagType b) noexcept                                  \
{                                                                                                 \
    return static_cast<TFlagType>(a.flags & b.flags);                                             \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType operator^(TFlagType a, TFlagType b) noexcept                                  \
{                                                                                                 \
    return static_cast<TFlagType>(a.flags ^ b.flags);                                             \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType operator~(TFlagType a) noexcept                                               \
{                                                                                                 \
    return static_cast<TFlagType>(~a.flags);                                                      \
}                                                                                                 \
constexpr TFlagType& operator|=(TFlagType& a, TFlagType b) noexcept                               \
{                                                                                                 \
    a.flags = (a.flags | b.flags);                                                                \
    return a;                                                                                     \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType& operator&=(TFlagType& a, TFlagType b) noexcept                               \
{                                                                                                 \
    a.flags = (a.flags & b.flags);                                                                \
    return a;                                                                                     \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType operator^=(TFlagType& a, TFlagType b) noexcept                                \
{                                                                                                 \
    a.flags = (a.flags ^ b.flags);                                                                \
    return a;                                                                                     \
}                                                                                                 \
constexpr TFlagType operator|(TFlagTypeBits a, TFlagTypeBits b) noexcept                          \
{                                                                                                 \
    return static_cast<TFlagType>(static_cast<TFlagBaseType>(a) | static_cast<TFlagBaseType>(b)); \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType operator&(TFlagTypeBits a, TFlagTypeBits b) noexcept                          \
{                                                                                                 \
    return static_cast<TFlagType>(static_cast<TFlagBaseType>(a) & static_cast<TFlagBaseType>(b)); \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType operator~(TFlagTypeBits key) noexcept                                         \
{                                                                                                 \
    return static_cast<TFlagType>(~static_cast<TFlagBaseType>(key));                              \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType operator^(TFlagTypeBits a, TFlagTypeBits b) noexcept                          \
{                                                                                                 \
    return static_cast<TFlagType>(static_cast<TFlagBaseType>(a) ^ static_cast<TFlagBaseType>(b)); \
}

template<class TTag>
struct TIdImpl {
    enum class Id : std::size_t { Invalid = SIZE_MAX };

    bool operator==(const TIdImpl&) const noexcept = default;
};

template<class TTag>
using TId = typename TIdImpl<TTag>::Id;

template<class TTag>
struct TIdGenerator {
public:
    auto Next() -> TId<TTag> {
        _counter += 1;
        return { _counter };
    }
private:
    size_t _counter = 0;
};

constexpr auto HashString(std::string_view str) -> uint32_t {
    auto hash = 2166136261u;
    for (auto ch: str) {
        hash ^= ch;
        hash *= 16777619u;
    }
    return hash;
}

constexpr auto operator"" _hash(const char* str, size_t) -> uint32_t {
    return HashString(str);
}

struct TExtent2D
{
    uint32_t Width{};
    uint32_t Height{};

    bool operator==(const TExtent2D&) const noexcept = default;
    constexpr TExtent2D operator+(const TExtent2D& other) const { return { Width + other.Width, Height + other.Height }; }
    constexpr TExtent2D operator-(const TExtent2D& other) const { return { Width - other.Width, Height - other.Height }; }
    constexpr TExtent2D operator*(const TExtent2D& other) const { return { Width * other.Width, Height * other.Height }; }
    constexpr TExtent2D operator/(const TExtent2D& other) const { return { Width / other.Width, Height / other.Height }; }
    constexpr TExtent2D operator>>(const TExtent2D& other) const { return { Width >> other.Width, Height >> other.Height }; }
    constexpr TExtent2D operator<<(const TExtent2D& other) const { return { Width << other.Width, Height << other.Height }; }
    constexpr TExtent2D operator+(uint32_t value) const { return *this + TExtent2D{ value, value }; }
    constexpr TExtent2D operator-(uint32_t value) const { return *this - TExtent2D{ value, value }; }
    constexpr TExtent2D operator*(uint32_t value) const { return *this * TExtent2D{ value, value }; }
    constexpr TExtent2D operator/(uint32_t value) const { return *this / TExtent2D{ value, value }; }
    constexpr TExtent2D operator>>(uint32_t value) const { return *this >> TExtent2D{ value, value }; }
    constexpr TExtent2D operator<<(uint32_t value) const { return *this << TExtent2D{ value, value }; }
};

constexpr TExtent2D operator+(uint32_t value, TExtent2D extent) {
    return extent + value;
}

constexpr TExtent2D operator-(uint32_t value, TExtent2D extent) {
    return extent - value;
}

constexpr TExtent2D operator*(uint32_t value, TExtent2D extent) {
    return extent * value;
}

constexpr TExtent2D operator/(uint32_t value, TExtent2D extent) {
    return extent / value;
}

constexpr TExtent2D operator>>(uint32_t value, TExtent2D extent) {
    return extent >> value;
}

constexpr TExtent2D operator<<(uint32_t value, TExtent2D extent) {
    return extent << value;
}

struct TExtent3D
{
    uint32_t Width{};
    uint32_t Height{};
    uint32_t Depth{};

    operator TExtent2D() const { return { Width, Height }; }
    bool operator==(const TExtent3D&) const noexcept = default;
    constexpr TExtent3D operator+(const TExtent3D& other) const { return { Width + other.Width, Height + other.Height, Depth + other.Depth }; }
    constexpr TExtent3D operator-(const TExtent3D& other) const { return { Width - other.Width, Height - other.Height, Depth - other.Depth }; }
    constexpr TExtent3D operator*(const TExtent3D& other) const { return { Width * other.Width, Height * other.Height, Depth * other.Depth }; }
    constexpr TExtent3D operator/(const TExtent3D& other) const { return { Width / other.Width, Height / other.Height, Depth / other.Depth }; }
    constexpr TExtent3D operator>>(const TExtent3D& other) const { return { Width >> other.Width, Height >> other.Height, Depth >> other.Depth }; }
    constexpr TExtent3D operator<<(const TExtent3D& other) const { return { Width << other.Width, Height << other.Height, Depth << other.Depth }; }
    constexpr TExtent3D operator+(uint32_t value) const { return *this + TExtent3D{ value, value, value }; }
    constexpr TExtent3D operator-(uint32_t value) const { return *this - TExtent3D{ value, value, value }; }
    constexpr TExtent3D operator*(uint32_t value) const { return *this * TExtent3D{ value, value, value }; }
    constexpr TExtent3D operator/(uint32_t value) const { return *this / TExtent3D{ value, value, value }; }
    constexpr TExtent3D operator>>(uint32_t value) const { return *this >> TExtent3D{ value, value, value }; }
    constexpr TExtent3D operator<<(uint32_t value) const { return *this << TExtent3D{ value, value, value }; }
};

constexpr TExtent3D operator+(uint32_t value, TExtent3D extent) {
    return extent + value;
}

constexpr TExtent3D operator-(uint32_t value, TExtent3D extent) {
    return extent - value;
}

constexpr TExtent3D operator*(uint32_t value, TExtent3D extent) {
    return extent * value;
}

constexpr TExtent3D operator/(uint32_t value, TExtent3D extent) {
    return extent / value;
}

constexpr TExtent3D operator>>(uint32_t value, TExtent3D extent) {
    return extent >> value;
}

constexpr TExtent3D operator<<(uint32_t value, TExtent3D extent) {
    return extent << value;
}

struct TOffset2D {

    uint32_t X{};
    uint32_t Y{};

    bool operator==(const TOffset2D&) const noexcept = default;
    constexpr TOffset2D operator+(const TOffset2D & other) const { return { X + other.X, Y + other.Y }; }
    constexpr TOffset2D operator-(const TOffset2D & other) const { return { X - other.X, Y - other.Y }; }
    constexpr TOffset2D operator*(const TOffset2D & other) const { return { X * other.X, Y * other.Y }; }
    constexpr TOffset2D operator/(const TOffset2D & other) const { return { X / other.X, Y / other.Y }; }
    constexpr TOffset2D operator>>(const TOffset2D & other) const { return { X >> other.X, Y >> other.Y }; }
    constexpr TOffset2D operator<<(const TOffset2D & other) const { return { X << other.X, Y << other.Y }; }
    constexpr TOffset2D operator+(uint32_t value) const { return *this + TOffset2D{ value, value }; }
    constexpr TOffset2D operator-(uint32_t value) const { return *this - TOffset2D{ value, value }; }
    constexpr TOffset2D operator*(uint32_t value) const { return *this * TOffset2D{ value, value }; }
    constexpr TOffset2D operator/(uint32_t value) const { return *this / TOffset2D{ value, value }; }
    constexpr TOffset2D operator>>(uint32_t value) const { return *this >> TOffset2D{ value, value }; }
    constexpr TOffset2D operator<<(uint32_t value) const { return *this << TOffset2D{ value, value }; }
};

constexpr TOffset2D operator+(uint32_t value, TOffset2D offset) {
    return offset + value;
}

constexpr TOffset2D operator-(uint32_t value, TOffset2D offset) {
    return offset - value;
}

constexpr TOffset2D operator*(uint32_t value, TOffset2D offset) {
    return offset * value;
}

constexpr TOffset2D operator/(uint32_t value, TOffset2D offset) {
    return offset / value;
}

constexpr TOffset2D operator>>(uint32_t value, TOffset2D offset) {
    return offset >> value;
}

constexpr TOffset2D operator<<(uint32_t value, TOffset2D offset) {
    return offset << value;
}

struct TOffset3D {

    uint32_t X{};
    uint32_t Y{};
    uint32_t Z{};

    operator TOffset2D() const { return { X, Y }; }
    bool operator==(const TOffset3D&) const noexcept = default;
    constexpr TOffset3D operator+(const TOffset3D& other) const { return { X + other.X, Y + other.Y, Z + other.Z }; }
    constexpr TOffset3D operator-(const TOffset3D& other) const { return { X - other.X, Y - other.Y, Z - other.Z }; }
    constexpr TOffset3D operator*(const TOffset3D& other) const { return { X * other.X, Y * other.Y, Z * other.Z }; }
    constexpr TOffset3D operator/(const TOffset3D& other) const { return { X / other.X, Y / other.Y, Z / other.Z }; }
    constexpr TOffset3D operator>>(const TOffset3D& other) const { return { X >> other.X, Y >> other.Y, Z >> other.Z }; }
    constexpr TOffset3D operator<<(const TOffset3D& other) const { return { X << other.X, Y << other.Y, Z << other.Z }; }
    constexpr TOffset3D operator+(uint32_t value) const { return *this + TOffset3D{ value, value, value }; }
    constexpr TOffset3D operator-(uint32_t value) const { return *this - TOffset3D{ value, value, value }; }
    constexpr TOffset3D operator*(uint32_t value) const { return *this * TOffset3D{ value, value, value }; }
    constexpr TOffset3D operator/(uint32_t value) const { return *this / TOffset3D{ value, value, value }; }
    constexpr TOffset3D operator>>(uint32_t value) const { return *this >> TOffset3D{ value, value, value }; }
    constexpr TOffset3D operator<<(uint32_t value) const { return *this << TOffset3D{ value, value, value }; }
};

constexpr TOffset3D operator+(uint32_t value, TOffset3D offset) {
    return offset + value;
}

constexpr TOffset3D operator-(uint32_t value, TOffset3D offset) {
    return offset - value;
}

constexpr TOffset3D operator*(uint32_t value, TOffset3D offset) {
    return offset * value;
}

constexpr TOffset3D operator/(uint32_t value, TOffset3D offset) {
    return offset / value;
}

constexpr TOffset3D operator>>(uint32_t value, TOffset3D offset) {
    return offset >> value;
}

constexpr TOffset3D operator<<(uint32_t value, TOffset3D offset) {
    return offset << value;
}

enum class TWindowStyle {
    Windowed,
    Fullscreen,
    FullscreenExclusive
};

struct TWindowSettings {
    int32_t ResolutionWidth;
    int32_t ResolutionHeight;
    float ResolutionScale;
    TWindowStyle WindowStyle;
    bool IsDebug;
    bool IsVSyncEnabled;
};

// - RHI ----------------------------------------------------------------------

enum class TFormat : uint32_t {

    Undefined,

    // Color formats
    R8_UNORM,
    R8_SNORM,
    R16_UNORM,
    R16_SNORM,
    R8G8_UNORM,
    R8G8_SNORM,
    R16G16_UNORM,
    R16G16_SNORM,
    R3G3B2_UNORM,
    R4G4B4_UNORM,
    R5G5B5_UNORM,
    R8G8B8_UNORM,
    R8G8B8_SNORM,
    R10G10B10_UNORM,
    R12G12B12_UNORM,
    R16G16B16_SNORM,
    R2G2B2A2_UNORM,
    R4G4B4A4_UNORM,
    R5G5B5A1_UNORM,
    R8G8B8A8_UNORM,
    R8G8B8A8_SNORM,
    R10G10B10A2_UNORM,
    R10G10B10A2_UINT,
    R12G12B12A12_UNORM,
    R16G16B16A16_UNORM,
    R16G16B16A16_SNORM,
    R8G8B8_SRGB,
    R8G8B8A8_SRGB,
    R16_FLOAT,
    R16G16_FLOAT,
    R16G16B16_FLOAT,
    R16G16B16A16_FLOAT,
    R32_FLOAT,
    R32G32_FLOAT,
    R32G32B32_FLOAT,
    R32G32B32A32_FLOAT,
    R11G11B10_FLOAT,
    R9G9B9_E5,
    R8_SINT,
    R8_UINT,
    R16_SINT,
    R16_UINT,
    R32_SINT,
    R32_UINT,
    R8G8_SINT,
    R8G8_UINT,
    R16G16_SINT,
    R16G16_UINT,
    R32G32_SINT,
    R32G32_UINT,
    R8G8B8_SINT,
    R8G8B8_UINT,
    R16G16B16_SINT,
    R16G16B16_UINT,
    R32G32B32_SINT,
    R32G32B32_UINT,
    R8G8B8A8_SINT,
    R8G8B8A8_UINT,
    R16G16B16A16_SINT,
    R16G16B16A16_UINT,
    R32G32B32A32_SINT,
    R32G32B32A32_UINT,

    // Depth & stencil formats
    D32_FLOAT,
    D32_UNORM,
    D24_UNORM,
    D16_UNORM,
    D32_FLOAT_S8_UINT,
    D24_UNORM_S8_UINT,
    S8_UINT,

    // Compressed formats
    // DXT
    BC1_RGB_UNORM,
    BC1_RGB_SRGB,
    BC1_RGBA_UNORM,
    BC1_RGBA_SRGB,
    BC2_RGBA_UNORM,
    BC2_RGBA_SRGB,
    BC3_RGBA_UNORM,
    BC3_RGBA_SRGB,
    // RGTC
    BC4_R_UNORM,
    BC4_R_SNORM,
    BC5_RG_UNORM,
    BC5_RG_SNORM,
    // BPTC
    BC6H_RGB_UFLOAT,
    BC6H_RGB_SFLOAT,
    BC7_RGBA_UNORM,
    BC7_RGBA_SRGB
};

enum class TFormatClass {
    Float,
    Integer,
    Long
};

enum class TBaseTypeClass {
    Float,
    Integer,
    UnsignedInteger
};

enum class TTextureType : uint32_t {
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture2DMultisample,
    Texture2DMultisampleArray,
    Texture3D,
    TextureCube,
    TextureCubeArray,
};

enum class TSampleCount : uint32_t {
    One = 1,
    Two = 2,
    Four = 4,
    Eight = 8,
    SixTeen = 16,
    ThirtyTwo = 32,
};

enum class TUploadFormat : uint32_t
{
    Undefined,
    Auto,
    R,
    Rg,
    Rgb,
    Bgr,
    Rgba,
    Bgra,
    RInteger,
    RgInteger,
    RgbInteger,
    BgrInteger,
    RgbaInteger,
    BgraInteger,
    Depth,
    StencilIndex,
    DepthStencilIndex,
};

enum class TUploadType : uint32_t
{
    Undefined,
    Auto,
    UnsignedByte,
    SignedByte,
    UnsignedShort,
    SignedShort,
    UnsignedInteger,
    SignedInteger,
    Float,
    UnsignedByte332,
    UnsignedByte233Reversed,
    UnsignedShort565,
    UnsignedShort565Reversed,
    UnsignedShort4444,
    UnsignedShort4444Reversed,
    UnsignedShort5551,
    UnsignedShort1555Reversed,
    UnsignedInteger8888,
    UnsignedInteger8888Reversed,
    UnsignedInteger1010102,
    UnsignedInteger2101010Reversed,
};

enum class TAttachmentType : uint32_t {
    ColorAttachment0 = 0u,
    ColorAttachment1,
    ColorAttachment2,
    ColorAttachment3,
    ColorAttachment4,
    ColorAttachment5,
    ColorAttachment6,
    ColorAttachment7,
    DepthAttachment,
    StencilAttachment
};

enum class TTextureAddressMode {
    Repeat,
    RepeatMirrored,
    ClampToEdge,
    ClampToBorder,
    ClampToEdgeMirrored,
};

enum class TTextureMinFilter {
    Nearest,
    NearestMipmapLinear,
    NearestMipmapNearest,
    Linear,
    LinearMipmapLinear,
    LinearMipmapNearest,
};

enum class TTextureMagFilter {
    Nearest,
    Linear
};

enum class TPrimitiveTopology {
    Triangles,
    TriangleStrip,
    TriangleFan,
    Lines,
};

enum class TFramebufferAttachmentLoadOperation {
    Load,
    Clear,
    DontCare
};

using TTextureId = TId<struct GTextureId>;
using TSamplerId = TId<struct GSamplerId>;
using TFramebufferId = TId<struct GFramebufferId>;
using TBufferId = TId<struct GBufferId>;
using TGraphicsPipelineId = TId<struct GGraphicsPipelineId>;
using TComputePipelineId = TId<struct GComputePipelineId>;

struct TCreateTextureDescriptor {
    TTextureType TextureType = {};
    TFormat Format = {};
    TExtent3D Extent = {};
    uint32_t MipMapLevels = 0;
    uint32_t Layers = 0;
    TSampleCount SampleCount = {};
    std::string Label = {};
};

struct TUploadTextureDescriptor {
    uint32_t Level;
    TOffset3D Offset;
    TExtent3D Extent;
    TUploadFormat UploadFormat = TUploadFormat::Auto;
    TUploadType UploadType = TUploadType::Auto;
    const void* PixelData = nullptr;
};

struct TTexture {
    uint32_t Id;
    TFormat Format;
    TExtent3D Extent;
    TTextureType TextureType;
};

struct TFramebufferAttachmentClearColor {
    TFramebufferAttachmentClearColor() = default;

    template<typename... Args>
    requires (sizeof...(Args) <= 4)
    TFramebufferAttachmentClearColor(const Args&... args)
      : Data(std::array<std::common_type_t<std::remove_cvref_t<Args>...>, 4>{ args...})
    {
    }

    std::variant<std::array<float, 4>, std::array<uint32_t, 4>, std::array<int32_t, 4>> Data;
};

struct TFramebufferAttachmentClearDepthStencil {
    float Depth = {};
    int32_t Stencil = {};
};

struct TFramebufferColorAttachmentDescriptor {
    std::string_view Label;
    TFormat Format;
    TExtent2D Extent;
    TFramebufferAttachmentLoadOperation LoadOperation;
    TFramebufferAttachmentClearColor ClearColor;
};

struct TFramebufferDepthStencilAttachmentDescriptor {
    std::string_view Label;
    TFormat Format;
    TExtent2D Extent;
    TFramebufferAttachmentLoadOperation LoadOperation;
    TFramebufferAttachmentClearDepthStencil ClearDepthStencil;
};

struct TFramebufferExistingDepthStencilAttachmentDescriptor {
    TTexture& ExistingDepthTexture;
};

struct TFramebufferDescriptor {
    std::string_view Label;
    std::array<std::optional<TFramebufferColorAttachmentDescriptor>, 8> ColorAttachments;
    std::optional<std::variant<TFramebufferDepthStencilAttachmentDescriptor, TFramebufferExistingDepthStencilAttachmentDescriptor>> DepthStencilAttachment;
};

struct TFramebufferColorAttachment {
    TTexture Texture;
    TFramebufferAttachmentClearColor ClearColor;
    TFramebufferAttachmentLoadOperation LoadOperation;
};

struct TFramebufferDepthStencilAttachment {
    TTexture Texture;
    TFramebufferAttachmentClearDepthStencil ClearDepthStencil;
    TFramebufferAttachmentLoadOperation LoadOperation;
};

struct TFramebuffer {
    uint32_t Id;
    std::array<std::optional<TFramebufferColorAttachment>, 8> ColorAttachments;
    std::optional<TFramebufferDepthStencilAttachment> DepthStencilAttachment;
};

struct TSamplerDescriptor {
    std::string_view Label;
    TTextureAddressMode AddressModeU = TTextureAddressMode::ClampToEdge;
    TTextureAddressMode AddressModeV = TTextureAddressMode::ClampToEdge;
    TTextureAddressMode AddressModeW = TTextureAddressMode::ClampToEdge;
    TTextureMagFilter MagFilter = TTextureMagFilter::Linear;
    TTextureMinFilter MinFilter = TTextureMinFilter::Linear;
    float LodBias = 0;
    float LodMin = -1000;
    float LodMax = 1000;

    bool operator==(const TSamplerDescriptor& rhs) const {
        return Label == rhs.Label &&
            AddressModeU == rhs.AddressModeU &&
            AddressModeV == rhs.AddressModeV &&
            AddressModeW == rhs.AddressModeW &&
            MagFilter == rhs.MagFilter &&
            MinFilter == rhs.MinFilter &&
            LodBias == rhs.LodBias &&
            LodMin == rhs.LodMin &&
            LodMax == rhs.LodMax;
    }
};

namespace std {
    template<>
    struct hash<TSamplerDescriptor> {
        size_t operator()(const TSamplerDescriptor& samplerDescriptor) const {
            size_t seed = 0;
            hash_combine(seed, samplerDescriptor.Label);
            hash_combine(seed, samplerDescriptor.AddressModeU);
            hash_combine(seed, samplerDescriptor.AddressModeV);
            hash_combine(seed, samplerDescriptor.AddressModeW);
            hash_combine(seed, samplerDescriptor.MagFilter);
            hash_combine(seed, samplerDescriptor.MinFilter);
            hash_combine(seed, samplerDescriptor.LodBias);
            hash_combine(seed, samplerDescriptor.LodMax);
            hash_combine(seed, samplerDescriptor.LodMin);
            return seed;
        }
    private:
        template<typename T>
        void hash_combine(size_t& seed, const T& v) const {
            std::hash<T> hasher;
            seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
    };
}

struct TSampler {
    uint32_t Id;
};

struct TInputAssemblyDescriptor {
    TPrimitiveTopology PrimitiveTopology = TPrimitiveTopology::Triangles;
    bool IsPrimitiveRestartEnabled = false;
};

struct TVertexInputAttributeDescriptor {
    uint32_t Location;
    uint32_t Binding;
    TFormat Format;
    uint32_t Offset;
};

struct TVertexInputDescriptor {
    std::array<std::optional<const TVertexInputAttributeDescriptor>, 16> VertexInputAttributes = {};
};

enum class TFillMode {
    Solid,
    Wireframe,
    Points
};

enum class TCullMode {
    None,
    Front,
    Back,
    FrontAndBack
};

enum class TFaceWindingOrder {
    Clockwise,
    CounterClockwise
};

enum class TDepthFunction {
    Never,
    Less,
    Equal,
    LessThanOrEqual,
    Greater,
    NotEqual,
    GreaterThanOrEqual,
    Always,
};

struct TClipControl {

};

struct TRasterizerState {
    TFillMode FillMode;
    TCullMode CullMode;
    TFaceWindingOrder FaceWindingOrder;
    bool IsDepthBiasEnabled;
    float DepthBiasSlopeFactor;
    float DepthBiasConstantFactor;
    bool IsDepthClampEnabled;
    bool IsScissorEnabled;
    TClipControl ClipControl;
    bool IsRasterizerDisabled;
    float LineWidth = 1.0f;
    float PointSize = 1.0f;
};

struct TBlendState {
    bool IsBlendEnabled;
};

struct TDepthState {
    bool IsDepthTestEnabled;
    bool IsDepthWriteEnabled;
    TDepthFunction DepthFunction;
};

struct TStencilState {

};

enum class TColorMaskBits {
    None = 0,
    R = 1 << 0,
    G = 1 << 1,
    B = 1 << 2,
    A = 1 << 3
};
DECLARE_FLAG_TYPE(TColorMask, TColorMaskBits, uint32_t)

struct TOutputMergerState {
    TColorMask ColorMask = TColorMaskBits::R | TColorMaskBits::G | TColorMaskBits::B | TColorMaskBits::A;
    TBlendState BlendState = {};
    TDepthState DepthState = {};
    TStencilState StencilState = {};
};

struct TGraphicsPipelineDescriptor {
    std::string_view Label;
    std::string_view VertexShaderFilePath;
    std::string_view FragmentShaderFilePath;

    TInputAssemblyDescriptor InputAssembly = {};
    std::optional<TVertexInputDescriptor> VertexInput;

    TRasterizerState RasterizerState = {};
    TOutputMergerState OutputMergerState = {};
};

struct TComputePipelineDescriptor {
    std::string_view Label;
    std::string_view ComputeShaderFilePath;
};

struct TPipeline {
    uint32_t Id;

    virtual auto Bind() -> void {
        glUseProgram(Id);
    }

    inline auto BindBufferAsUniformBuffer(uint32_t buffer, int32_t bindingIndex) -> void {
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingIndex, buffer);
    }

    inline auto BindBufferAsShaderStorageBuffer(uint32_t buffer, int32_t bindingIndex) -> void {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingIndex, buffer);
    }

    inline auto BindTexture(int32_t bindingIndex, uint32_t texture) -> void {
        glBindTextureUnit(bindingIndex, texture);
    }

    inline auto BindTextureAndSampler(int32_t bindingIndex, uint32_t texture, uint32_t sampler) -> void {
        glBindTextureUnit(bindingIndex, texture);
        glBindSampler(bindingIndex, sampler);
    }

    inline auto SetUniform(int32_t location, float value) -> void {
        glProgramUniform1f(Id, location, value);
    }

    inline auto SetUniform(int32_t location, int32_t value) -> void {
        glProgramUniform1i(Id, location, value);
    }

    inline auto SetUniform(int32_t location, uint32_t value) -> void {
        glProgramUniform1ui(Id, location, value);
    }

    inline auto SetUniform(int32_t location, uint64_t value) -> void {
        glProgramUniformHandleui64ARB(Id, location, value); //TODO(deccer) add check for bindless support
    }

    inline auto SetUniform(int32_t location, const glm::vec2& value) -> void {
        glProgramUniform2fv(Id, location, 1, glm::value_ptr(value));
    }

    inline auto SetUniform(int32_t location, const glm::vec3& value) -> void {
        glProgramUniform3fv(Id, location, 1, glm::value_ptr(value));
    }

    inline auto SetUniform(int32_t location, float value1, float value2, float value3, float value4) -> void {
        glProgramUniform4f(Id, location, value1, value2, value3, value4);
    }

    inline auto SetUniform(int32_t location, int32_t value1, int32_t value2, int32_t value3, int32_t value4) -> void {
        glProgramUniform4i(Id, location, value1, value2, value3, value4);
    }

    inline auto SetUniform(int32_t location, const glm::vec4 value) -> void {
        glProgramUniform4fv(Id, location, 1, glm::value_ptr(value));
    }

    inline auto SetUniform(int32_t location, const glm::mat4& value) -> void {
        glProgramUniformMatrix4fv(Id, location, 1, GL_FALSE, glm::value_ptr(value));
    }
};

struct TGraphicsPipeline : public TPipeline {

    auto Bind() -> void override;
    auto BindBufferAsVertexBuffer(uint32_t buffer, uint32_t bindingIndex, size_t offset, size_t stride) -> void;
    auto DrawArrays(int32_t vertexOffset, size_t vertexCount) -> void;
    auto DrawElements(uint32_t indexBuffer, size_t elementCount) -> void;
    auto DrawElementsInstanced(uint32_t indexBuffer, size_t elementCount, size_t instanceCount) -> void;

    // Input Assembly
    std::optional<uint32_t> InputLayout;
    uint32_t PrimitiveTopology;
    bool IsPrimitiveRestartEnabled;

    // Rasterizer Stage
    TFillMode FillMode;
    TCullMode CullMode;
    TFaceWindingOrder FaceWindingOrder;
    bool IsDepthBiasEnabled;
    float DepthBiasSlopeFactor;
    float DepthBiasConstantFactor;
    bool IsDepthClampEnabled;
    bool IsScissorEnabled;
    TClipControl ClipControl;
    bool IsRasterizerDisabled;
    float LineWidth;
    float PointSize;

    // Output Merger State
    TColorMask ColorMask;
    bool IsDepthTestEnabled;
    bool IsDepthWriteEnabled;
    TDepthFunction DepthFunction;
};

struct TComputePipeline : public TPipeline {

};

inline auto SetDebugLabel(
    const uint32_t object,
    const uint32_t objectType,
    const std::string_view label) -> void {

    glObjectLabel(objectType, object, static_cast<GLsizei>(label.size()), label.data());
}

inline auto PushDebugGroup(const std::string_view label) -> void {
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, static_cast<GLsizei>(label.size()), label.data());
}

inline auto PopDebugGroup() -> void {
    glPopDebugGroup();
}

inline auto CreateBuffer(
    std::string_view label,
    size_t sizeInBytes,
    const void* data,
    uint32_t flags) -> uint32_t {

    uint32_t buffer = 0;
    glCreateBuffers(1, &buffer);
    SetDebugLabel(buffer, GL_BUFFER, label);
    glNamedBufferStorage(buffer, sizeInBytes, data, flags);
    return buffer;
}

inline auto UpdateBuffer(
    uint32_t buffer,
    size_t offsetInBytes,
    size_t sizeInBytes,
    const void* data) -> void {

    glNamedBufferSubData(buffer, offsetInBytes, sizeInBytes, data);
}

inline auto DeleteBuffer(uint32_t buffer) -> void {

    glDeleteBuffers(1, &buffer);
}

inline auto DeletePipeline(TPipeline pipeline) -> void {

    glDeleteProgram(pipeline.Id);
}

uint32_t g_defaultInputLayout = 0;
uint32_t g_lastIndexBuffer = 0;

std::vector<TSampler> g_samplers;
TSamplerId g_samplerCounter = TSamplerId::Invalid;

std::vector<TTexture> g_textures;
TTextureId g_textureCounter = TTextureId::Invalid;

std::unordered_map<TSamplerDescriptor, TSamplerId> g_samplerDescriptors;

auto InitializeRhi() -> bool {

    if (gladLoadGL(glfwGetProcAddress) == GL_FALSE) {
        return false;
    }

    g_samplers.reserve(128);
    g_textures.reserve(16384);    

    glCreateVertexArrays(1, &g_defaultInputLayout);
    SetDebugLabel(g_defaultInputLayout, GL_VERTEX_ARRAY, "InputLayout-Default");

    return true;
}

auto UnloadRhi() -> void {

    glDeleteVertexArrays(1, &g_defaultInputLayout);
}

auto ReadShaderSourceFromFile(const std::filesystem::path& filePath) -> std::expected<std::string, std::string> {

    std::string pathStr = filePath.string();
    std::string parentPathStr = filePath.parent_path().string();
    char error[256]{};
    auto processedSource = std::unique_ptr<char, decltype([](char* ptr) { free(ptr); })>(stb_include_file(pathStr.data(), nullptr, parentPathStr.data(), error));
    if (!processedSource)
    {
        return std::unexpected(std::format("Failed to process includes for {}", filePath.string()));
    }

    return processedSource.get();
}

inline auto GetTexture(TTextureId id) -> TTexture& {

    assert(id != TTextureId::Invalid);
    return g_textures[size_t(id)];
}

///// Conversions

constexpr auto PrimitiveTopologyToGL(TPrimitiveTopology primitiveTopology) -> uint32_t {
    switch (primitiveTopology) {
        case TPrimitiveTopology::Lines: return GL_LINES;
        case TPrimitiveTopology::TriangleFan: return GL_TRIANGLE_FAN;
        case TPrimitiveTopology::Triangles: return GL_TRIANGLES;
        case TPrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
        default: std::unreachable();
    }
}

constexpr auto TextureAddressModeToGL(TTextureAddressMode textureAddressMode) -> uint32_t {
    switch (textureAddressMode) {
        case TTextureAddressMode::ClampToBorder : return GL_CLAMP_TO_BORDER;
        case TTextureAddressMode::ClampToEdge : return GL_CLAMP_TO_EDGE;
        case TTextureAddressMode::ClampToEdgeMirrored: return GL_MIRROR_CLAMP_TO_EDGE;
        case TTextureAddressMode::Repeat: return GL_REPEAT;
        case TTextureAddressMode::RepeatMirrored: return GL_MIRRORED_REPEAT;
        default: std::unreachable();
    }
}

constexpr auto TextureMagFilterToGL(TTextureMagFilter textureMagFilter) -> uint32_t {
    switch (textureMagFilter) {
        case TTextureMagFilter::Linear: return GL_LINEAR;
        case TTextureMagFilter::Nearest: return GL_NEAREST;
        default: std::unreachable();
    }
}

constexpr auto TextureMinFilterToGL(TTextureMinFilter textureMinFilter) -> uint32_t {
    switch (textureMinFilter) {
        case TTextureMinFilter::Linear: return GL_LINEAR;
        case TTextureMinFilter::Nearest: return GL_NEAREST;
        case TTextureMinFilter::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR;
        case TTextureMinFilter::LinearMipmapNearest: return GL_LINEAR_MIPMAP_NEAREST;
        case TTextureMinFilter::NearestMipmapLinear: return GL_NEAREST_MIPMAP_LINEAR;
        case TTextureMinFilter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
        default: std::unreachable();
    }
}

constexpr auto TextureTypeToGL(TTextureType textureType) -> uint32_t {

    switch (textureType)
    {
        case TTextureType::Texture1D: return GL_TEXTURE_1D;
        case TTextureType::Texture2D: return GL_TEXTURE_2D;
        case TTextureType::Texture3D: return GL_TEXTURE_3D;
        case TTextureType::Texture1DArray: return GL_TEXTURE_1D_ARRAY;
        case TTextureType::Texture2DArray: return GL_TEXTURE_2D_ARRAY;
        case TTextureType::TextureCube: return GL_TEXTURE_CUBE_MAP;
        case TTextureType::TextureCubeArray: return GL_TEXTURE_CUBE_MAP_ARRAY;
        case TTextureType::Texture2DMultisample: return GL_TEXTURE_2D_MULTISAMPLE;
        case TTextureType::Texture2DMultisampleArray: return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
        default: std::unreachable();
    }
}

constexpr auto UploadFormatToGL(TUploadFormat uploadFormat) -> uint32_t {

    switch (uploadFormat) {
        case TUploadFormat::R: return GL_RED;
        case TUploadFormat::Rg: return GL_RG;
        case TUploadFormat::Rgb: return GL_RGB;
        case TUploadFormat::Bgr: return GL_BGR;
        case TUploadFormat::Rgba: return GL_RGBA;
        case TUploadFormat::Bgra: return GL_BGRA;
        case TUploadFormat::RInteger: return GL_RED_INTEGER;
        case TUploadFormat::RgInteger: return GL_RG_INTEGER;
        case TUploadFormat::RgbInteger: return GL_RGB_INTEGER;
        case TUploadFormat::BgrInteger: return GL_BGR_INTEGER;
        case TUploadFormat::RgbaInteger: return GL_RGBA_INTEGER;
        case TUploadFormat::BgraInteger: return GL_BGRA_INTEGER;
        case TUploadFormat::Depth: return GL_DEPTH_COMPONENT;
        case TUploadFormat::StencilIndex: return GL_STENCIL_INDEX;
        case TUploadFormat::DepthStencilIndex: return GL_DEPTH_STENCIL;
        default: std::unreachable();
    }
}

constexpr auto UploadTypeToGL(TUploadType uploadType) -> uint32_t {

    switch (uploadType) {
        case TUploadType::UnsignedByte: return GL_UNSIGNED_BYTE;
        case TUploadType::SignedByte: return GL_BYTE;
        case TUploadType::UnsignedShort: return GL_UNSIGNED_SHORT;
        case TUploadType::SignedShort: return GL_SHORT;
        case TUploadType::UnsignedInteger: return GL_UNSIGNED_INT;
        case TUploadType::SignedInteger: return GL_INT;
        case TUploadType::Float: return GL_FLOAT;
        case TUploadType::UnsignedByte332: return GL_UNSIGNED_BYTE_3_3_2;
        case TUploadType::UnsignedByte233Reversed: return GL_UNSIGNED_BYTE_2_3_3_REV;
        case TUploadType::UnsignedShort565: return GL_UNSIGNED_SHORT_5_6_5;
        case TUploadType::UnsignedShort565Reversed: return GL_UNSIGNED_SHORT_5_6_5_REV;
        case TUploadType::UnsignedShort4444: return GL_UNSIGNED_SHORT_4_4_4_4;
        case TUploadType::UnsignedShort4444Reversed: return GL_UNSIGNED_SHORT_4_4_4_4_REV;
        case TUploadType::UnsignedShort5551: return GL_UNSIGNED_SHORT_5_5_5_1;
        case TUploadType::UnsignedShort1555Reversed: return GL_UNSIGNED_SHORT_1_5_5_5_REV;
        case TUploadType::UnsignedInteger8888: return GL_UNSIGNED_INT_8_8_8_8;
        case TUploadType::UnsignedInteger8888Reversed: return GL_UNSIGNED_INT_8_8_8_8_REV;
        case TUploadType::UnsignedInteger1010102: return GL_UNSIGNED_INT_10_10_10_2;
        case TUploadType::UnsignedInteger2101010Reversed: return GL_UNSIGNED_INT_2_10_10_10_REV;
        default: std::unreachable();
    }
}

constexpr auto SampleCountToGL(TSampleCount sampleCount) -> uint32_t {

    switch (sampleCount) {
        case TSampleCount::One: return 1;
        case TSampleCount::Two: return 2;
        case TSampleCount::Four: return 4;
        case TSampleCount::Eight: return 8;
        case TSampleCount::SixTeen: return 16;
        case TSampleCount::ThirtyTwo: return 32;
        default: std::unreachable();
    }
}

auto FormatToGL(TFormat format) -> uint32_t {
    switch (format) {
        case TFormat::R8_UNORM: return GL_R8;
        case TFormat::R8_SNORM: return GL_R8_SNORM;
        case TFormat::R16_UNORM: return GL_R16;
        case TFormat::R16_SNORM: return GL_R16_SNORM;
        case TFormat::R8G8_UNORM: return GL_RG8;
        case TFormat::R8G8_SNORM: return GL_RG8_SNORM;
        case TFormat::R16G16_UNORM: return GL_RG16;
        case TFormat::R16G16_SNORM: return GL_RG16_SNORM;
        case TFormat::R3G3B2_UNORM: return GL_R3_G3_B2;
        case TFormat::R4G4B4_UNORM: return GL_RGB4;
        case TFormat::R5G5B5_UNORM: return GL_RGB5;
        case TFormat::R8G8B8_UNORM: return GL_RGB8;
        case TFormat::R8G8B8_SNORM: return GL_RGB8_SNORM;
        case TFormat::R10G10B10_UNORM: return GL_RGB10;
        case TFormat::R12G12B12_UNORM: return GL_RGB12;
        // GL_RG16?
        case TFormat::R16G16B16_SNORM: return GL_RGB16_SNORM;
        case TFormat::R2G2B2A2_UNORM: return GL_RGBA2;
        case TFormat::R4G4B4A4_UNORM: return GL_RGBA4;
        case TFormat::R5G5B5A1_UNORM: return GL_RGB5_A1;
        case TFormat::R8G8B8A8_UNORM: return GL_RGBA8;
        case TFormat::R8G8B8A8_SNORM: return GL_RGBA8_SNORM;
        case TFormat::R10G10B10A2_UNORM: return GL_RGB10_A2;
        case TFormat::R10G10B10A2_UINT: return GL_RGB10_A2UI;
        case TFormat::R12G12B12A12_UNORM: return GL_RGBA12;
        case TFormat::R16G16B16A16_UNORM: return GL_RGBA16;
        case TFormat::R16G16B16A16_SNORM: return GL_RGBA16_SNORM;
        case TFormat::R8G8B8_SRGB: return GL_SRGB8;
        case TFormat::R8G8B8A8_SRGB: return GL_SRGB8_ALPHA8;
        case TFormat::R16_FLOAT: return GL_R16F;
        case TFormat::R16G16_FLOAT: return GL_RG16F;
        case TFormat::R16G16B16_FLOAT: return GL_RGB16F;
        case TFormat::R16G16B16A16_FLOAT: return GL_RGBA16F;
        case TFormat::R32_FLOAT: return GL_R32F;
        case TFormat::R32G32_FLOAT: return GL_RG32F;
        case TFormat::R32G32B32_FLOAT: return GL_RGB32F;
        case TFormat::R32G32B32A32_FLOAT: return GL_RGBA32F;
        case TFormat::R11G11B10_FLOAT: return GL_R11F_G11F_B10F;
        case TFormat::R9G9B9_E5: return GL_RGB9_E5;
        case TFormat::R8_SINT: return GL_R8I;
        case TFormat::R8_UINT: return GL_R8UI;
        case TFormat::R16_SINT: return GL_R16I;
        case TFormat::R16_UINT: return GL_R16UI;
        case TFormat::R32_SINT: return GL_R32I;
        case TFormat::R32_UINT: return GL_R32UI;
        case TFormat::R8G8_SINT: return GL_RG8I;
        case TFormat::R8G8_UINT: return GL_RG8UI;
        case TFormat::R16G16_SINT: return GL_RG16I;
        case TFormat::R16G16_UINT: return GL_RG16UI;
        case TFormat::R32G32_SINT: return GL_RG32I;
        case TFormat::R32G32_UINT: return GL_RG32UI;
        case TFormat::R8G8B8_SINT: return GL_RGB8I;
        case TFormat::R8G8B8_UINT: return GL_RGB8UI;
        case TFormat::R16G16B16_SINT: return GL_RGB16I;
        case TFormat::R16G16B16_UINT: return GL_RGB16UI;
        case TFormat::R32G32B32_SINT: return GL_RGB32I;
        case TFormat::R32G32B32_UINT: return GL_RGB32UI;
        case TFormat::R8G8B8A8_SINT: return GL_RGBA8I;
        case TFormat::R8G8B8A8_UINT: return GL_RGBA8UI;
        case TFormat::R16G16B16A16_SINT: return GL_RGBA16I;
        case TFormat::R16G16B16A16_UINT: return GL_RGBA16UI;
        case TFormat::R32G32B32A32_SINT: return GL_RGBA32I;
        case TFormat::R32G32B32A32_UINT: return GL_RGBA32UI;
        case TFormat::D32_FLOAT: return GL_DEPTH_COMPONENT32F;
        case TFormat::D32_UNORM: return GL_DEPTH_COMPONENT32;
        case TFormat::D24_UNORM: return GL_DEPTH_COMPONENT24;
        case TFormat::D16_UNORM: return GL_DEPTH_COMPONENT16;
        case TFormat::D32_FLOAT_S8_UINT: return GL_DEPTH32F_STENCIL8;
        case TFormat::D24_UNORM_S8_UINT: return GL_DEPTH24_STENCIL8;
        case TFormat::S8_UINT: return GL_STENCIL_INDEX8;
        /*
        case EFormat::BC1_RGB_UNORM: return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        case EFormat::BC1_RGBA_UNORM: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        case EFormat::BC1_RGB_SRGB: return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
        case EFormat::BC1_RGBA_SRGB: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
        case EFormat::BC2_RGBA_UNORM: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        case EFormat::BC2_RGBA_SRGB: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
        case EFormat::BC3_RGBA_UNORM: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        case EFormat::BC3_RGBA_SRGB: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
        */
        case TFormat::BC4_R_UNORM: return GL_COMPRESSED_RED_RGTC1;
        case TFormat::BC4_R_SNORM: return GL_COMPRESSED_SIGNED_RED_RGTC1;
        case TFormat::BC5_RG_UNORM: return GL_COMPRESSED_RG_RGTC2;
        case TFormat::BC5_RG_SNORM: return GL_COMPRESSED_SIGNED_RG_RGTC2;
        case TFormat::BC6H_RGB_UFLOAT: return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
        case TFormat::BC6H_RGB_SFLOAT: return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
        case TFormat::BC7_RGBA_UNORM: return GL_COMPRESSED_RGBA_BPTC_UNORM;
        case TFormat::BC7_RGBA_SRGB: return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
        default:
            std::string message = "Format not mappable";
            glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, message.size(), message.data());
            return -1;
    }
}

auto AttachmentTypeToGL(TAttachmentType attachmentType) -> uint32_t {
    switch (attachmentType) {
        case TAttachmentType::ColorAttachment0: return GL_COLOR_ATTACHMENT0;
        case TAttachmentType::ColorAttachment1: return GL_COLOR_ATTACHMENT1;
        case TAttachmentType::ColorAttachment2: return GL_COLOR_ATTACHMENT2;
        case TAttachmentType::ColorAttachment3: return GL_COLOR_ATTACHMENT3;
        case TAttachmentType::ColorAttachment4: return GL_COLOR_ATTACHMENT4;
        case TAttachmentType::ColorAttachment5: return GL_COLOR_ATTACHMENT5;
        case TAttachmentType::ColorAttachment6: return GL_COLOR_ATTACHMENT6;
        case TAttachmentType::ColorAttachment7: return GL_COLOR_ATTACHMENT7;
        case TAttachmentType::DepthAttachment: return GL_DEPTH_ATTACHMENT;
        case TAttachmentType::StencilAttachment: return GL_STENCIL_ATTACHMENT;
        default:
            std::string message = "AttachmentType not mappable";
            glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, message.size(), message.data());
            return -1;        
    }
}

auto FormatToUploadFormat(TFormat format) -> TUploadFormat {

    switch (format)
    {
        case TFormat::R8_UNORM:
        case TFormat::R8_SNORM:
        case TFormat::R16_UNORM:
        case TFormat::R16_SNORM:
        case TFormat::R16_FLOAT:
        case TFormat::R32_FLOAT:
            return TUploadFormat::R;
        case TFormat::R8_SINT:
        case TFormat::R8_UINT:
        case TFormat::R16_SINT:
        case TFormat::R16_UINT:
        case TFormat::R32_SINT:
        case TFormat::R32_UINT:
            return TUploadFormat::RInteger;
        case TFormat::R8G8_UNORM:
        case TFormat::R8G8_SNORM:
        case TFormat::R16G16_UNORM:
        case TFormat::R16G16_SNORM:
        case TFormat::R16G16_FLOAT:
        case TFormat::R32G32_FLOAT:
            return TUploadFormat::Rg;
        case TFormat::R8G8_SINT:
        case TFormat::R8G8_UINT:
        case TFormat::R16G16_SINT:
        case TFormat::R16G16_UINT:
        case TFormat::R32G32_SINT:
        case TFormat::R32G32_UINT:
            return TUploadFormat::RgInteger;
        case TFormat::R3G3B2_UNORM:
        case TFormat::R4G4B4_UNORM:
        case TFormat::R5G5B5_UNORM:
        case TFormat::R8G8B8_UNORM:
        case TFormat::R8G8B8_SNORM:
        case TFormat::R10G10B10_UNORM:
        case TFormat::R12G12B12_UNORM:
        case TFormat::R16G16B16_SNORM:
        case TFormat::R8G8B8_SRGB:
        case TFormat::R16G16B16_FLOAT:
        case TFormat::R9G9B9_E5:
        case TFormat::R32G32B32_FLOAT:
        case TFormat::R11G11B10_FLOAT:
            return TUploadFormat::Rgb;
        case TFormat::R8G8B8_SINT:
        case TFormat::R8G8B8_UINT:
        case TFormat::R16G16B16_SINT:
        case TFormat::R16G16B16_UINT:
        case TFormat::R32G32B32_SINT:
        case TFormat::R32G32B32_UINT:
            return TUploadFormat::RgbInteger;
        case TFormat::R2G2B2A2_UNORM:
        case TFormat::R4G4B4A4_UNORM:
        case TFormat::R5G5B5A1_UNORM:
        case TFormat::R8G8B8A8_UNORM:
        case TFormat::R8G8B8A8_SNORM:
        case TFormat::R10G10B10A2_UNORM:
        case TFormat::R12G12B12A12_UNORM:
        case TFormat::R16G16B16A16_UNORM:
        case TFormat::R16G16B16A16_SNORM:
        case TFormat::R8G8B8A8_SRGB:
        case TFormat::R16G16B16A16_FLOAT:
        case TFormat::R32G32B32A32_FLOAT:
            return TUploadFormat::Rgba;
        case TFormat::R10G10B10A2_UINT:
        case TFormat::R8G8B8A8_SINT:
        case TFormat::R8G8B8A8_UINT:
        case TFormat::R16G16B16A16_SINT:
        case TFormat::R16G16B16A16_UINT:
        case TFormat::R32G32B32A32_SINT:
        case TFormat::R32G32B32A32_UINT:
            return TUploadFormat::RgbaInteger;
        case TFormat::D32_FLOAT:
        case TFormat::D32_UNORM:
        case TFormat::D24_UNORM:
        case TFormat::D16_UNORM:
            return TUploadFormat::Depth;
        case TFormat::D32_FLOAT_S8_UINT:
        case TFormat::D24_UNORM_S8_UINT:
            return TUploadFormat::DepthStencilIndex;
        case TFormat::S8_UINT:
            return TUploadFormat::StencilIndex;
        default: 
            std::unreachable();
	}
}

auto FormatToUnderlyingOpenGLType(TFormat format) -> uint32_t {

    switch (format)
    {
        case TFormat::R8_UNORM:
        case TFormat::R8G8_UNORM:
        case TFormat::R8G8B8_UNORM:
        case TFormat::R8G8B8A8_UNORM:
        case TFormat::R8_UINT:
        case TFormat::R8G8_UINT:
        case TFormat::R8G8B8_UINT:
        case TFormat::R8G8B8A8_UINT:
        case TFormat::R8G8B8A8_SRGB:
        case TFormat::R8G8B8_SRGB:
            return GL_UNSIGNED_BYTE;
        case TFormat::R8_SNORM:
        case TFormat::R8G8_SNORM:
        case TFormat::R8G8B8_SNORM:
        case TFormat::R8G8B8A8_SNORM:
        case TFormat::R8_SINT:
        case TFormat::R8G8_SINT:
        case TFormat::R8G8B8_SINT:
        case TFormat::R8G8B8A8_SINT:
            return GL_BYTE;
        case TFormat::R16_UNORM:
        case TFormat::R16G16_UNORM:
        case TFormat::R16G16B16A16_UNORM:
        case TFormat::R16_UINT:
        case TFormat::R16G16_UINT:
        case TFormat::R16G16B16_UINT:
        case TFormat::R16G16B16A16_UINT:
            return GL_UNSIGNED_SHORT;
        case TFormat::R16_SNORM:
        case TFormat::R16G16_SNORM:
        case TFormat::R16G16B16_SNORM:
        case TFormat::R16G16B16A16_SNORM:
        case TFormat::R16_SINT:
        case TFormat::R16G16_SINT:
        case TFormat::R16G16B16_SINT:
        case TFormat::R16G16B16A16_SINT:
            return GL_SHORT;
        case TFormat::R16_FLOAT:
        case TFormat::R16G16_FLOAT:
        case TFormat::R16G16B16_FLOAT:
        case TFormat::R16G16B16A16_FLOAT:
            return GL_HALF_FLOAT;
        case TFormat::R32_FLOAT:
        case TFormat::R32G32_FLOAT:
        case TFormat::R32G32B32_FLOAT:
        case TFormat::R32G32B32A32_FLOAT:
            return GL_FLOAT;
        case TFormat::R32_SINT:
        case TFormat::R32G32_SINT:
        case TFormat::R32G32B32_SINT:
        case TFormat::R32G32B32A32_SINT:
            return GL_INT;
        case TFormat::R32_UINT:
        case TFormat::R32G32_UINT:
        case TFormat::R32G32B32_UINT:
        case TFormat::R32G32B32A32_UINT:
            return GL_UNSIGNED_INT;
        default:
            std::string message = "Format not mappable to opengl type";
            glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, message.size(), message.data());
            return -1;
    }
}

constexpr auto TextureTypeToDimension(TTextureType textureType) -> uint32_t {

    switch (textureType) {
        case TTextureType::Texture1D:
            return 1;
        case TTextureType::Texture2D:
        case TTextureType::Texture2DMultisample:
        case TTextureType::Texture1DArray:
            return 2;
        case TTextureType::Texture3D:
        case TTextureType::Texture2DArray:
        case TTextureType::Texture2DMultisampleArray:
        case TTextureType::TextureCube:
        case TTextureType::TextureCubeArray:
            return 3;
        default:
            std::unreachable();
    } 
}

auto FormatToBaseTypeClass(TFormat format) -> TBaseTypeClass
{
    switch (format)
    {
        case TFormat::R8_UNORM:
        case TFormat::R8_SNORM:
        case TFormat::R16_UNORM:
        case TFormat::R16_SNORM:
        case TFormat::R8G8_UNORM:
        case TFormat::R8G8_SNORM:
        case TFormat::R16G16_UNORM:
        case TFormat::R16G16_SNORM:
        case TFormat::R3G3B2_UNORM:
        case TFormat::R4G4B4_UNORM:
        case TFormat::R5G5B5_UNORM:
        case TFormat::R8G8B8_UNORM:
        case TFormat::R8G8B8_SNORM:
        case TFormat::R10G10B10_UNORM:
        case TFormat::R12G12B12_UNORM:
        case TFormat::R16G16B16_SNORM:
        case TFormat::R2G2B2A2_UNORM:
        case TFormat::R4G4B4A4_UNORM:
        case TFormat::R5G5B5A1_UNORM:
        case TFormat::R8G8B8A8_UNORM:
        case TFormat::R8G8B8A8_SNORM:
        case TFormat::R10G10B10A2_UNORM:
        case TFormat::R12G12B12A12_UNORM:
        case TFormat::R16G16B16A16_UNORM:
        case TFormat::R8G8B8_SRGB:
        case TFormat::R8G8B8A8_SRGB:
        case TFormat::R16_FLOAT:
        case TFormat::R16G16_FLOAT:
        case TFormat::R16G16B16_FLOAT:
        case TFormat::R16G16B16A16_FLOAT:
        case TFormat::R32_FLOAT:
        case TFormat::R32G32_FLOAT:
        case TFormat::R32G32B32_FLOAT:
        case TFormat::R32G32B32A32_FLOAT:
        case TFormat::R11G11B10_FLOAT:
        case TFormat::R9G9B9_E5:
            return TBaseTypeClass::Float;
        case TFormat::R8_SINT:
        case TFormat::R16_SINT:
        case TFormat::R32_SINT:
        case TFormat::R8G8_SINT:
        case TFormat::R16G16_SINT:
        case TFormat::R32G32_SINT:
        case TFormat::R8G8B8_SINT:
        case TFormat::R16G16B16_SINT:
        case TFormat::R32G32B32_SINT:
        case TFormat::R8G8B8A8_SINT:
        case TFormat::R16G16B16A16_SINT:
        case TFormat::R32G32B32A32_SINT:
            return TBaseTypeClass::Integer;
        case TFormat::R10G10B10A2_UINT:
        case TFormat::R8_UINT:
        case TFormat::R16_UINT:
        case TFormat::R32_UINT:
        case TFormat::R8G8_UINT:
        case TFormat::R16G16_UINT:
        case TFormat::R32G32_UINT:
        case TFormat::R8G8B8_UINT:
        case TFormat::R16G16B16_UINT:
        case TFormat::R32G32B32_UINT:
        case TFormat::R8G8B8A8_UINT:
        case TFormat::R16G16B16A16_UINT:
        case TFormat::R32G32B32A32_UINT:
            return TBaseTypeClass::UnsignedInteger;
        default:
            std::unreachable();
    }
}

constexpr auto FormatToAttachmentType(TFormat attachmentFormat, size_t colorAttachmentIndex) -> TAttachmentType {

    switch (attachmentFormat) {
        case TFormat::D24_UNORM:
        case TFormat::D24_UNORM_S8_UINT:
        case TFormat::D32_FLOAT:
        case TFormat::D32_FLOAT_S8_UINT:
        case TFormat::D32_UNORM:
            return TAttachmentType::DepthAttachment;
        default:
            return static_cast<TAttachmentType>(static_cast<std::underlying_type<TAttachmentType>::type>(TAttachmentType::ColorAttachment0) + colorAttachmentIndex);
    }
}

auto FormatToComponentCount(TFormat format) -> int32_t {
    switch (format) {
        case TFormat::R8_UNORM:
        case TFormat::R8_SNORM:
        case TFormat::R16_UNORM:
        case TFormat::R16_SNORM:
        case TFormat::R16_FLOAT:
        case TFormat::R32_FLOAT:
        case TFormat::R8_SINT:
        case TFormat::R16_SINT:
        case TFormat::R32_SINT:
        case TFormat::R8_UINT:
        case TFormat::R16_UINT:
        case TFormat::R32_UINT:
            return 1;
        case TFormat::R8G8_UNORM:
        case TFormat::R8G8_SNORM:
        case TFormat::R16G16_FLOAT:
        case TFormat::R16G16_UNORM:
        case TFormat::R16G16_SNORM:
        case TFormat::R32G32_FLOAT:
        case TFormat::R8G8_SINT:
        case TFormat::R16G16_SINT:
        case TFormat::R32G32_SINT:
        case TFormat::R8G8_UINT:
        case TFormat::R16G16_UINT:
        case TFormat::R32G32_UINT:
            return 2;
        case TFormat::R8G8B8_UNORM:
        case TFormat::R8G8B8_SNORM:
        case TFormat::R16G16B16_SNORM:
        case TFormat::R16G16B16_FLOAT:
        case TFormat::R32G32B32_FLOAT:
        case TFormat::R8G8B8_SINT:
        case TFormat::R16G16B16_SINT:
        case TFormat::R32G32B32_SINT:
        case TFormat::R8G8B8_UINT:
        case TFormat::R16G16B16_UINT:
        case TFormat::R32G32B32_UINT:
            return 3;
        case TFormat::R8G8B8A8_UNORM:
        case TFormat::R8G8B8A8_SNORM:
        case TFormat::R16G16B16A16_UNORM:
        case TFormat::R16G16B16A16_FLOAT:
        case TFormat::R32G32B32A32_FLOAT:
        case TFormat::R8G8B8A8_SINT:
        case TFormat::R16G16B16A16_SINT:
        case TFormat::R32G32B32A32_SINT:
        case TFormat::R10G10B10A2_UINT:
        case TFormat::R8G8B8A8_UINT:
        case TFormat::R16G16B16A16_UINT:
        case TFormat::R32G32B32A32_UINT:
            return 4;
        default:
            std::unreachable();
    }
}

auto IsFormatNormalized(TFormat format) -> int32_t {

    switch (format) {
        case TFormat::R8_UNORM:
        case TFormat::R8_SNORM:
        case TFormat::R16_UNORM:
        case TFormat::R16_SNORM:
        case TFormat::R8G8_UNORM:
        case TFormat::R8G8_SNORM:
        case TFormat::R16G16_UNORM:
        case TFormat::R16G16_SNORM:
        case TFormat::R8G8B8_UNORM:
        case TFormat::R8G8B8_SNORM:
        case TFormat::R16G16B16_SNORM:
        case TFormat::R8G8B8A8_UNORM:
        case TFormat::R8G8B8A8_SNORM:
        case TFormat::R16G16B16A16_UNORM:
            return GL_TRUE;
        case TFormat::R16_FLOAT:
        case TFormat::R32_FLOAT:
        case TFormat::R8_SINT:
        case TFormat::R16_SINT:
        case TFormat::R32_SINT:
        case TFormat::R8_UINT:
        case TFormat::R16_UINT:
        case TFormat::R32_UINT:
        case TFormat::R16G16_FLOAT:
        case TFormat::R32G32_FLOAT:
        case TFormat::R8G8_SINT:
        case TFormat::R16G16_SINT:
        case TFormat::R32G32_SINT:
        case TFormat::R8G8_UINT:
        case TFormat::R16G16_UINT:
        case TFormat::R32G32_UINT:
        case TFormat::R16G16B16_FLOAT:
        case TFormat::R32G32B32_FLOAT:
        case TFormat::R8G8B8_SINT:
        case TFormat::R16G16B16_SINT:
        case TFormat::R32G32B32_SINT:
        case TFormat::R8G8B8_UINT:
        case TFormat::R16G16B16_UINT:
        case TFormat::R32G32B32_UINT:
        case TFormat::R16G16B16A16_FLOAT:
        case TFormat::R32G32B32A32_FLOAT:
        case TFormat::R8G8B8A8_SINT:
        case TFormat::R16G16B16A16_SINT:
        case TFormat::R32G32B32A32_SINT:
        case TFormat::R10G10B10A2_UINT:
        case TFormat::R8G8B8A8_UINT:
        case TFormat::R16G16B16A16_UINT:
        case TFormat::R32G32B32A32_UINT:
            return GL_FALSE;
        default:
            std::unreachable();
    }
}

auto FormatToFormatClass(TFormat format) -> TFormatClass
  {
    switch (format)
    {
        case TFormat::R8_UNORM:
        case TFormat::R8_SNORM:
        case TFormat::R16_UNORM:
        case TFormat::R16_SNORM:
        case TFormat::R8G8_UNORM:
        case TFormat::R8G8_SNORM:
        case TFormat::R16G16_UNORM:
        case TFormat::R16G16_SNORM:
        case TFormat::R8G8B8_UNORM:
        case TFormat::R8G8B8_SNORM:
        case TFormat::R16G16B16_SNORM:
        case TFormat::R8G8B8A8_UNORM:
        case TFormat::R8G8B8A8_SNORM:
        case TFormat::R16G16B16A16_UNORM:
        case TFormat::R16_FLOAT:
        case TFormat::R16G16_FLOAT:
        case TFormat::R16G16B16_FLOAT:
        case TFormat::R16G16B16A16_FLOAT:
        case TFormat::R32_FLOAT:
        case TFormat::R32G32_FLOAT:
        case TFormat::R32G32B32_FLOAT:
        case TFormat::R32G32B32A32_FLOAT:
            return TFormatClass::Float;
        case TFormat::R8_SINT:
        case TFormat::R16_SINT:
        case TFormat::R32_SINT:
        case TFormat::R8G8_SINT:
        case TFormat::R16G16_SINT:
        case TFormat::R32G32_SINT:
        case TFormat::R8G8B8_SINT:
        case TFormat::R16G16B16_SINT:
        case TFormat::R32G32B32_SINT:
        case TFormat::R8G8B8A8_SINT:
        case TFormat::R16G16B16A16_SINT:
        case TFormat::R32G32B32A32_SINT:
        case TFormat::R10G10B10A2_UINT:
        case TFormat::R8_UINT:
        case TFormat::R16_UINT:
        case TFormat::R32_UINT:
        case TFormat::R8G8_UINT:
        case TFormat::R16G16_UINT:
        case TFormat::R32G32_UINT:
        case TFormat::R8G8B8_UINT:
        case TFormat::R16G16B16_UINT:
        case TFormat::R32G32B32_UINT:
        case TFormat::R8G8B8A8_UINT:
        case TFormat::R16G16B16A16_UINT:
        case TFormat::R32G32B32A32_UINT:
            return TFormatClass::Integer;
        default: 
            return TFormatClass::Long;
    }
}

/////////////////////////////////////////////////////////////

auto FreeImage(void* pixels) -> void {
    if (pixels != nullptr) {
        stbi_image_free(pixels);
    }
}

auto LoadImageFromMemory(
    std::byte* encodedData,
    size_t encodedDataSize,
    int32_t* width,
    int32_t* height,
    int32_t* components) -> unsigned char* {

    return stbi_load_from_memory(
        reinterpret_cast<const unsigned char*>(encodedData),
        static_cast<int32_t>(encodedDataSize),
        width,
        height,
        components,
        4);
}

auto LoadImageFromFile(
    const std::filesystem::path& filePath,
    int32_t* width,
    int32_t* height,
    int32_t* components) -> unsigned char* {

    PROFILER_ZONESCOPEDN("LoadImageFromFile");

    auto imageFile = fopen(filePath.c_str(), "rb");
    if (imageFile != nullptr) {
        auto* pixels = stbi_load_from_file(imageFile, width, height, components, 4);
        fclose(imageFile);
        return pixels;
    } else {
        return nullptr;
    }
}

auto CreateTexture(const TCreateTextureDescriptor& createTextureDescriptor) -> TTextureId {

    PROFILER_ZONESCOPEDN("CreateTexture");
    TTexture texture = {};

    glCreateTextures(TextureTypeToGL(createTextureDescriptor.TextureType), 1, &texture.Id);
    if (!createTextureDescriptor.Label.empty()) {
        SetDebugLabel(texture.Id, GL_TEXTURE, createTextureDescriptor.Label);
    }

    texture.Extent = createTextureDescriptor.Extent;
    texture.Format = createTextureDescriptor.Format;
    texture.TextureType = createTextureDescriptor.TextureType;

    switch (createTextureDescriptor.TextureType)
    {
        case TTextureType::Texture1D:
            glTextureStorage1D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width);
            break;
        case TTextureType::Texture2D:
            glTextureStorage2D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height);
            break;
        case TTextureType::Texture3D:
            glTextureStorage3D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height,
                               createTextureDescriptor.Extent.Depth);
            break;
        case TTextureType::Texture1DArray:
            glTextureStorage2D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Layers);
            break;
        case TTextureType::Texture2DArray:
            glTextureStorage3D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height,
                               createTextureDescriptor.Layers);
            break;
        case TTextureType::TextureCube:
            glTextureStorage2D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height);
            break;
        case TTextureType::TextureCubeArray:
            glTextureStorage3D(texture.Id,
                               createTextureDescriptor.MipMapLevels,
                               FormatToGL(createTextureDescriptor.Format),
                               createTextureDescriptor.Extent.Width,
                               createTextureDescriptor.Extent.Height,
                               createTextureDescriptor.Layers);
            break;
        case TTextureType::Texture2DMultisample:
            glTextureStorage2DMultisample(texture.Id,
                                          SampleCountToGL(createTextureDescriptor.SampleCount),
                                          FormatToGL(createTextureDescriptor.Format),
                                          createTextureDescriptor.Extent.Width,
                                          createTextureDescriptor.Extent.Height,
                                          GL_TRUE);
            break;
        case TTextureType::Texture2DMultisampleArray:
            glTextureStorage3DMultisample(texture.Id,
                                          SampleCountToGL(createTextureDescriptor.SampleCount),
                                          FormatToGL(createTextureDescriptor.Format),
                                          createTextureDescriptor.Extent.Width,
                                          createTextureDescriptor.Extent.Height,
                                          createTextureDescriptor.Layers,
                                          GL_TRUE);
            break;
        default:
            std::unreachable();
    }

    const auto textureId = TTextureId(g_textures.size());
    g_textures.emplace_back(texture);    

    return textureId;
}

auto UploadTexture(const TTextureId& textureId, const TUploadTextureDescriptor& updateTextureDescriptor) -> void {

    PROFILER_ZONESCOPEDN("UploadTexture");
    auto& texture = GetTexture(textureId);

    uint32_t format = 0;
    if (updateTextureDescriptor.UploadFormat == TUploadFormat::Auto)
    {
        format = UploadFormatToGL(FormatToUploadFormat(texture.Format));
    }
    else
    {
        format = UploadFormatToGL(updateTextureDescriptor.UploadFormat);
    }

    uint32_t type = 0;
    if (updateTextureDescriptor.UploadType == TUploadType::Auto)
    {
        type = FormatToUnderlyingOpenGLType(texture.Format);
    }
    else
    {
        type = UploadTypeToGL(updateTextureDescriptor.UploadType);
    }

    switch (TextureTypeToDimension(texture.TextureType))
    {
        case 1:
            glTextureSubImage1D(texture.Id,
                                updateTextureDescriptor.Level,
                                updateTextureDescriptor.Offset.X,
                                updateTextureDescriptor.Extent.Width,
                                format,
                                type,
                                updateTextureDescriptor.PixelData);
            break;
        case 2:
            glTextureSubImage2D(texture.Id,
                                updateTextureDescriptor.Level,
                                updateTextureDescriptor.Offset.X,
                                updateTextureDescriptor.Offset.Y,
                                updateTextureDescriptor.Extent.Width,
                                updateTextureDescriptor.Extent.Height,
                                format,
                                type,
                                updateTextureDescriptor.PixelData);
            break;
        case 3:
            glTextureSubImage3D(texture.Id,
                                updateTextureDescriptor.Level,
                                updateTextureDescriptor.Offset.X,
                                updateTextureDescriptor.Offset.Y,
                                updateTextureDescriptor.Offset.Z,
                                updateTextureDescriptor.Extent.Width,
                                updateTextureDescriptor.Extent.Height,
                                updateTextureDescriptor.Extent.Depth,
                                format,
                                type,
                                updateTextureDescriptor.PixelData);
            break;
    }
}

auto MakeTextureResident(const TTextureId& textureId) -> uint64_t {
    
    auto& texture = GetTexture(textureId);

    auto textureHandle = glGetTextureHandleARB(texture.Id);
    glMakeTextureHandleResidentARB(textureHandle);

    return textureHandle;
}

auto GenerateMipmaps(const TTextureId textureId) -> void {

    auto& texture = GetTexture(textureId);
    glGenerateTextureMipmap(texture.Id);
}

auto DeleteTextures() -> void {
    for(auto& texture : g_textures) {
        glDeleteTextures(1, &texture.Id);
    }
}

auto GetOrCreateSampler(const TSamplerDescriptor& samplerDescriptor) -> TSamplerId {

    if (auto existingSamplerDescriptor = g_samplerDescriptors.find(samplerDescriptor); existingSamplerDescriptor != g_samplerDescriptors.end()) {
        return existingSamplerDescriptor->second;
    }

    TSampler sampler = {};
    glCreateSamplers(1, &sampler.Id);
    SetDebugLabel(sampler.Id, GL_SAMPLER, samplerDescriptor.Label);
    glSamplerParameteri(sampler.Id, GL_TEXTURE_WRAP_S, TextureAddressModeToGL(samplerDescriptor.AddressModeU));
    glSamplerParameteri(sampler.Id, GL_TEXTURE_WRAP_T, TextureAddressModeToGL(samplerDescriptor.AddressModeV));
    glSamplerParameteri(sampler.Id, GL_TEXTURE_WRAP_R, TextureAddressModeToGL(samplerDescriptor.AddressModeW));
    glSamplerParameteri(sampler.Id, GL_TEXTURE_MAG_FILTER, TextureMagFilterToGL(samplerDescriptor.MagFilter));
    glSamplerParameteri(sampler.Id, GL_TEXTURE_MIN_FILTER, TextureMinFilterToGL(samplerDescriptor.MinFilter));
    glSamplerParameterf(sampler.Id, GL_TEXTURE_LOD_BIAS, samplerDescriptor.LodBias);
    glSamplerParameteri(sampler.Id, GL_TEXTURE_MIN_LOD, samplerDescriptor.LodMin);
    glSamplerParameteri(sampler.Id, GL_TEXTURE_MAX_LOD, samplerDescriptor.LodMax);

    const auto samplerId = TSamplerId(g_samplers.size());
    g_samplerDescriptors.insert({samplerDescriptor, samplerId});
    g_samplers.emplace_back(sampler);

    return samplerId;
}

auto CreateFramebuffer(const TFramebufferDescriptor& framebufferDescriptor) -> TFramebuffer {

    PROFILER_ZONESCOPEDN("CreateFramebuffer");

    TFramebuffer framebuffer = {};
    glCreateFramebuffers(1, &framebuffer.Id);
    if (!framebufferDescriptor.Label.empty()) {
        SetDebugLabel(framebuffer.Id, GL_FRAMEBUFFER, framebufferDescriptor.Label);
    }

    std::array<uint32_t, 8> drawBuffers = {};

    for (auto colorAttachmentIndex = 0; auto colorAttachmentDescriptorValue : framebufferDescriptor.ColorAttachments) {
        if (colorAttachmentDescriptorValue.has_value()) {
            auto& colorAttachmentDescriptor = *colorAttachmentDescriptorValue;
            auto colorAttachmentTextureId = CreateTexture({
                .TextureType = TTextureType::Texture2D,
                .Format = colorAttachmentDescriptor.Format,
                .Extent = TExtent3D(colorAttachmentDescriptor.Extent.Width, colorAttachmentDescriptor.Extent.Height, 1),
                .MipMapLevels = 1,
                .Layers = 0,
                .SampleCount = TSampleCount::One,
                .Label = std::format("{}_{}x{}", colorAttachmentDescriptor.Label, colorAttachmentDescriptor.Extent.Width, colorAttachmentDescriptor.Extent.Height)
            });
            auto& colorAttachmentTexture = GetTexture(colorAttachmentTextureId);

            framebuffer.ColorAttachments[colorAttachmentIndex] = {
                .Texture = colorAttachmentTexture,
                .ClearColor = colorAttachmentDescriptor.ClearColor,
                .LoadOperation = colorAttachmentDescriptor.LoadOperation,
            };

            auto attachmentType = FormatToAttachmentType(colorAttachmentDescriptor.Format, colorAttachmentIndex);
            glNamedFramebufferTexture(framebuffer.Id, AttachmentTypeToGL(attachmentType), colorAttachmentTexture.Id, 0);

            drawBuffers[colorAttachmentIndex] = AttachmentTypeToGL(attachmentType);

        } else {
            framebuffer.ColorAttachments[colorAttachmentIndex] = std::nullopt;
            drawBuffers[colorAttachmentIndex] = GL_NONE;
        }

        colorAttachmentIndex++;        
    }

    glNamedFramebufferDrawBuffers(framebuffer.Id, 8, drawBuffers.data());

    if (framebufferDescriptor.DepthStencilAttachment.has_value()) {
        auto& depthStencilAttachment = *framebufferDescriptor.DepthStencilAttachment;
        if (auto* createDepthStencilAttachment = std::get_if<TFramebufferDepthStencilAttachmentDescriptor>(&depthStencilAttachment)) {
            auto depthTextureId = CreateTexture({
                .TextureType = TTextureType::Texture2D,
                .Format = createDepthStencilAttachment->Format,
                .Extent = TExtent3D(createDepthStencilAttachment->Extent.Width,
                                    createDepthStencilAttachment->Extent.Height, 1),
                .MipMapLevels = 1,
                .Layers = 0,
                .SampleCount = TSampleCount::One,
                .Label = std::format("{}_{}x{}", createDepthStencilAttachment->Label,
                                     createDepthStencilAttachment->Extent.Width,
                                     createDepthStencilAttachment->Extent.Height)
            });
            auto& depthTexture = g_textures[size_t(depthTextureId)];

            auto attachmentType = FormatToAttachmentType(createDepthStencilAttachment->Format, 0);
            glNamedFramebufferTexture(framebuffer.Id, AttachmentTypeToGL(attachmentType), depthTexture.Id, 0);

            framebuffer.DepthStencilAttachment = {
                .Texture = depthTexture,
                .ClearDepthStencil = createDepthStencilAttachment->ClearDepthStencil,
                .LoadOperation = createDepthStencilAttachment->LoadOperation,
            };
        } else if (auto* existingDepthStencilAttachment = std::get_if<TFramebufferExistingDepthStencilAttachmentDescriptor>(&depthStencilAttachment)) {
            auto& depthTexture = existingDepthStencilAttachment->ExistingDepthTexture;
            glNamedFramebufferTexture(framebuffer.Id, GL_DEPTH_ATTACHMENT, depthTexture.Id, 0);
            framebuffer.DepthStencilAttachment = {
                .Texture = depthTexture,
                .LoadOperation = TFramebufferAttachmentLoadOperation::DontCare,
            };
        }
    } else {
        framebuffer.DepthStencilAttachment = std::nullopt;
    }

    auto framebufferStatus = glCheckNamedFramebufferStatus(framebuffer.Id, GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
        auto message = std::format("Framebuffer {} is incomplete", framebufferDescriptor.Label);
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 1, GL_DEBUG_SEVERITY_HIGH, message.size(), message.data());
    }

    return framebuffer;
}

auto BindFramebuffer(const TFramebuffer& framebuffer) -> void {

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.Id);

    auto hasColorAttachment = true;
    for (auto colorAttachmentIndex = 0; auto colorAttachmentValue : framebuffer.ColorAttachments) {
        if (colorAttachmentValue.has_value()) {
            auto& colorAttachment = *colorAttachmentValue;
            glViewportIndexedf(colorAttachmentIndex, 0, 0, colorAttachment.Texture.Extent.Width, colorAttachment.Texture.Extent.Height);
            glColorMaski(colorAttachmentIndex, true, true, true, true);
            if (colorAttachment.LoadOperation == TFramebufferAttachmentLoadOperation::Clear) {
                auto baseTypeClass = FormatToBaseTypeClass(colorAttachment.Texture.Format);
                switch (baseTypeClass) {
                    case TBaseTypeClass::Float:
                        glClearNamedFramebufferfv(framebuffer.Id, GL_COLOR, colorAttachmentIndex, std::get_if<std::array<float, 4>>(&colorAttachment.ClearColor.Data)->data());
                        break;
                    case TBaseTypeClass::Integer:
                        glClearNamedFramebufferiv(framebuffer.Id, GL_COLOR, colorAttachmentIndex, std::get_if<std::array<int32_t, 4>>(&colorAttachment.ClearColor.Data)->data());
                        break;
                    case TBaseTypeClass::UnsignedInteger:
                        glClearNamedFramebufferuiv(framebuffer.Id, GL_COLOR, colorAttachmentIndex, std::get_if<std::array<uint32_t, 4>>(&colorAttachment.ClearColor.Data)->data());
                        break;
                    default:
                        std::unreachable_sentinel;
                }
            }
        } else {
            hasColorAttachment = false;
        }
        colorAttachmentIndex++;
    }

    if (framebuffer.DepthStencilAttachment.has_value()) {
        auto& depthStencilAttachment = *framebuffer.DepthStencilAttachment;
        if (!hasColorAttachment) {
            glViewport(0, 0, depthStencilAttachment.Texture.Extent.Width, depthStencilAttachment.Texture.Extent.Height);
        }
        if (depthStencilAttachment.LoadOperation == TFramebufferAttachmentLoadOperation::Clear) {
            glDepthMask(GL_TRUE);
            glStencilMask(GL_TRUE);
            glClearNamedFramebufferfi(framebuffer.Id, GL_DEPTH_STENCIL, 0, depthStencilAttachment.ClearDepthStencil.Depth, depthStencilAttachment.ClearDepthStencil.Stencil);
        }
    }
}

auto DeleteFramebuffer(const TFramebuffer& framebuffer) -> void {

    assert(framebuffer.Id != 0);

    for (auto colorAttachment : framebuffer.ColorAttachments) {
        if (colorAttachment.has_value()) {
            glDeleteTextures(1, &(*colorAttachment).Texture.Id);
        }
    }

    if (framebuffer.DepthStencilAttachment.has_value()) {
        glDeleteTextures(1, &(*framebuffer.DepthStencilAttachment).Texture.Id);
    }

    glDeleteFramebuffers(1, &framebuffer.Id);
}

auto CreateGraphicsProgram(
    std::string_view label,
    std::string_view vertexShaderFilePath,
    std::string_view fragmentShaderFilePath) -> std::expected<uint32_t, std::string> {

    PROFILER_ZONESCOPEDN("CreateGraphicsProgram");

    auto vertexShaderSourceResult = ReadShaderSourceFromFile(vertexShaderFilePath);
    if (!vertexShaderSourceResult) {
        return std::unexpected(vertexShaderSourceResult.error());
    }

    auto fragmentShaderSourceResult = ReadShaderSourceFromFile(fragmentShaderFilePath);
    if (!fragmentShaderSourceResult) {
        return std::unexpected(fragmentShaderSourceResult.error());
    }

    auto vertexShaderSource = *vertexShaderSourceResult;
    auto fragmentShaderSource = *fragmentShaderSourceResult;

    auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
    auto vertexShaderSourcePtr = vertexShaderSource.data();
    glShaderSource(vertexShader, 1, &vertexShaderSourcePtr, nullptr);
    glCompileShader(vertexShader);
    int32_t status = 0;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {

        int32_t infoLength = 512;
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetShaderInfoLog(vertexShader, infoLength, nullptr, infoLog.data());
        glDeleteShader(vertexShader);

        return std::unexpected(std::format("Vertex shader in program {} has errors\n{}", label, infoLog));
    }

    auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    auto fragmentShaderSourcePtr = fragmentShaderSource.data();
    glShaderSource(fragmentShader, 1, &fragmentShaderSourcePtr, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {

        int32_t infoLength = 512;
        glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetShaderInfoLog(fragmentShader, infoLength, nullptr, infoLog.data());
        glDeleteShader(fragmentShader);

        return std::unexpected(std::format("Fragment shader in program {} has errors\n{}", label, infoLog));
    }

    auto program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (status == GL_FALSE) {

        int32_t infoLength = 512;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetProgramInfoLog(program, infoLength, nullptr, infoLog.data());
        glDeleteProgram(program);

        return std::unexpected(std::format("Graphics program {} has linking errors\n{}", label, infoLog));
    }

    return program;
}

auto CreateComputeProgram(
    std::string_view label,
    std::string_view computeShaderFilePath) -> std::expected<uint32_t, std::string> {

    PROFILER_ZONESCOPEDN("CreateComputeProgram");

    auto computeShaderSourceResult = ReadShaderSourceFromFile(computeShaderFilePath);
    if (!computeShaderSourceResult) {
        return std::unexpected(computeShaderSourceResult.error());
    }

    int32_t status = 0;

    auto computeShader = glCreateShader(GL_COMPUTE_SHADER);
    auto computeShaderSourcePtr = (*computeShaderSourceResult).data();
    glShaderSource(computeShader, 1, &computeShaderSourcePtr, nullptr);
    glCompileShader(computeShader);
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {

        int32_t infoLength = 512;
        glGetShaderiv(computeShader, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetShaderInfoLog(computeShader, infoLength, nullptr, infoLog.data());
        glDeleteShader(computeShader);

        return std::unexpected(std::format("Compute shader in program {} has errors\n{}", label, infoLog));
    }

    auto program = glCreateProgram();
    glAttachShader(program, computeShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    glDetachShader(program, computeShader);
    glDeleteShader(computeShader);

    if (status == GL_FALSE) {

        int32_t infoLength = 512;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength);
        auto infoLog = std::string(infoLength + 1, '\0');
        glGetProgramInfoLog(program, infoLength, nullptr, infoLog.data());

        return std::unexpected(std::format("Compute program {} has linking errors\n{}", label, infoLog));
    }

    return program;
}

auto FillModeToGL(TFillMode fillMode) -> uint32_t {
    switch (fillMode) {
        case TFillMode::Solid: return GL_FILL;
        case TFillMode::Wireframe: return GL_LINE;
        case TFillMode::Points: return GL_POINT;
        default: std::unreachable();
    }
}

auto CullModeToGL(TCullMode cullMode) -> uint32_t {
    switch (cullMode) {
        case TCullMode::None: return GL_NONE;
        case TCullMode::Back: return GL_BACK;
        case TCullMode::Front: return GL_FRONT;
        case TCullMode::FrontAndBack: return GL_FRONT_AND_BACK;
        default: std::unreachable();
    }
}

auto FaceWindingOrderToGL(TFaceWindingOrder faceWindingOrder) -> uint32_t {
    switch (faceWindingOrder) {
        case TFaceWindingOrder::CounterClockwise: return GL_CCW;
        case TFaceWindingOrder::Clockwise: return GL_CW;
        default: std::unreachable();
    }
}

auto DepthFunctionToGL(TDepthFunction depthFunction) -> uint32_t {
    switch (depthFunction) {
        case TDepthFunction::Never: return GL_NEVER;
        case TDepthFunction::Less: return GL_LESS;
        case TDepthFunction::Equal: return GL_EQUAL;
        case TDepthFunction::LessThanOrEqual: return GL_LEQUAL;
        case TDepthFunction::Greater: return GL_GREATER;
        case TDepthFunction::NotEqual: return GL_NOTEQUAL;
        case TDepthFunction::GreaterThanOrEqual: return GL_GEQUAL;
        case TDepthFunction::Always: return GL_ALWAYS;
        default: std::unreachable();
    }
}

auto CreateGraphicsPipeline(const TGraphicsPipelineDescriptor& graphicsPipelineDescriptor) -> std::expected<TGraphicsPipeline, std::string> {

    PROFILER_ZONESCOPEDN("CreateGraphicsPipeline");

    TGraphicsPipeline pipeline = {};

    auto graphicsProgramResult = CreateGraphicsProgram(graphicsPipelineDescriptor.Label,
                                                       graphicsPipelineDescriptor.VertexShaderFilePath,
                                                       graphicsPipelineDescriptor.FragmentShaderFilePath);
    if (!graphicsProgramResult) {
        return std::unexpected(std::format("RHI: Unable to build GraphicsPipeline {}\n{}", graphicsPipelineDescriptor.Label, graphicsProgramResult.error()));
    }

    pipeline.Id = *graphicsProgramResult;

    if (graphicsPipelineDescriptor.VertexInput.has_value()) {

        uint32_t inputLayout = 0;
        glCreateVertexArrays(1, &inputLayout);

        auto& vertexInput = *graphicsPipelineDescriptor.VertexInput;
        for(auto inputAttributeIndex = 0; auto& inputAttribute : vertexInput.VertexInputAttributes) {
            if (inputAttribute.has_value()) {
                auto& inputAttributeValue = *inputAttribute;
                
                glEnableVertexArrayAttrib(inputLayout, inputAttributeValue.Location);
                glVertexArrayAttribBinding(inputLayout, inputAttributeValue.Location, inputAttributeValue.Binding);

                auto type = FormatToUnderlyingOpenGLType(inputAttributeValue.Format);
                auto componentCount = FormatToComponentCount(inputAttributeValue.Format);
                auto isFormatNormalized = IsFormatNormalized(inputAttributeValue.Format);
                auto formatClass = FormatToFormatClass(inputAttributeValue.Format);
                switch (formatClass)
                {
                    case TFormatClass::Float:
                        glVertexArrayAttribFormat(inputLayout, inputAttributeValue.Location, componentCount, type, isFormatNormalized, inputAttributeValue.Offset);
                        break;
                    case TFormatClass::Integer:
                        glVertexArrayAttribIFormat(inputLayout, inputAttributeValue.Location, componentCount, type, inputAttributeValue.Offset);
                        break;
                    case TFormatClass::Long:
                        glVertexArrayAttribLFormat(inputLayout, inputAttributeValue.Location, componentCount, type, inputAttributeValue.Offset);
                        break;
                    default: 
                        std::string message = "Unsupported Format Class";
                        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, message.size(), message.data());
                        break;
                }
            }

            inputAttributeIndex++;
        }

        SetDebugLabel(inputLayout, GL_VERTEX_ARRAY, std::format("InputLayout-{}", graphicsPipelineDescriptor.Label));

        pipeline.InputLayout = inputLayout;
    } else {
        pipeline.InputLayout = std::nullopt;
    }
    // Input Assembly
    pipeline.PrimitiveTopology = PrimitiveTopologyToGL(graphicsPipelineDescriptor.InputAssembly.PrimitiveTopology);
    pipeline.IsPrimitiveRestartEnabled = graphicsPipelineDescriptor.InputAssembly.IsPrimitiveRestartEnabled;
    // Rasterizer Stage
    pipeline.FillMode = graphicsPipelineDescriptor.RasterizerState.FillMode;
    pipeline.CullMode = graphicsPipelineDescriptor.RasterizerState.CullMode;
    pipeline.FaceWindingOrder = graphicsPipelineDescriptor.RasterizerState.FaceWindingOrder;
    pipeline.IsDepthBiasEnabled = graphicsPipelineDescriptor.RasterizerState.IsDepthBiasEnabled;
    pipeline.DepthBiasSlopeFactor = graphicsPipelineDescriptor.RasterizerState.DepthBiasSlopeFactor;
    pipeline.DepthBiasConstantFactor = graphicsPipelineDescriptor.RasterizerState.DepthBiasConstantFactor;
    pipeline.IsDepthClampEnabled = graphicsPipelineDescriptor.RasterizerState.IsDepthClampEnabled;
    pipeline.ClipControl = graphicsPipelineDescriptor.RasterizerState.ClipControl;
    pipeline.IsRasterizerDisabled = graphicsPipelineDescriptor.RasterizerState.IsRasterizerDisabled;
    pipeline.LineWidth = graphicsPipelineDescriptor.RasterizerState.LineWidth;
    pipeline.PointSize = graphicsPipelineDescriptor.RasterizerState.PointSize;
    // Output Merger Stage
    pipeline.ColorMask = graphicsPipelineDescriptor.OutputMergerState.ColorMask;
    pipeline.IsDepthTestEnabled = graphicsPipelineDescriptor.OutputMergerState.DepthState.IsDepthTestEnabled;
    pipeline.IsDepthWriteEnabled = graphicsPipelineDescriptor.OutputMergerState.DepthState.IsDepthWriteEnabled;
    pipeline.DepthFunction = graphicsPipelineDescriptor.OutputMergerState.DepthState.DepthFunction;

    return pipeline;
}

auto CreateComputePipeline(const TComputePipelineDescriptor& computePipelineDescriptor) -> std::expected<TComputePipeline, std::string> {

    PROFILER_ZONESCOPEDN("CreateComputePipeline");

    TComputePipeline pipeline = {};

    auto computeProgramResult = CreateComputeProgram(computePipelineDescriptor.Label,
                                                     computePipelineDescriptor.ComputeShaderFilePath);
    if (!computeProgramResult) {
        return std::unexpected(std::format("RHI: Unable to build ComputePipeline {}\n{}", computePipelineDescriptor.Label, computeProgramResult.error()));
    }

    pipeline.Id = *computeProgramResult;

    return pipeline;
}

auto TGraphicsPipeline::Bind() -> void {

    TPipeline::Bind();
    // Input Assembly
    glBindVertexArray(InputLayout.has_value() ? *InputLayout : g_defaultInputLayout);

    // Rasterizer Stage
    glPolygonMode(GL_FRONT_AND_BACK, FillModeToGL(FillMode));
    if (CullMode == TCullMode::None) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(CullModeToGL(CullMode));
    }

    if (IsRasterizerDisabled) {
        glEnable(GL_RASTERIZER_DISCARD);
    }
    if (IsScissorEnabled) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }

    glFrontFace(FaceWindingOrderToGL(FaceWindingOrder));
    if (IsDepthBiasEnabled) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_POLYGON_OFFSET_LINE);
        glEnable(GL_POLYGON_OFFSET_POINT);

        glPolygonOffset(DepthBiasSlopeFactor, DepthBiasConstantFactor);
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_POLYGON_OFFSET_LINE);
        glDisable(GL_POLYGON_OFFSET_POINT);
    }

    glLineWidth(LineWidth);
    glPointSize(PointSize);

    // Output Merger Stage

    if (IsDepthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(DepthFunctionToGL(DepthFunction));
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    /*
    glDepthMask(IsDepthWriteEnabled ? GL_TRUE : GL_FALSE);
    glColorMask((ColorMask & TColorMaskBits::R) == TColorMaskBits::R,
                (ColorMask & TColorMaskBits::G) == TColorMaskBits::G,
                (ColorMask & TColorMaskBits::B) == TColorMaskBits::B,
                (ColorMask & TColorMaskBits::A) == TColorMaskBits::A);
    */
}

auto TGraphicsPipeline::BindBufferAsVertexBuffer(uint32_t buffer, uint32_t bindingIndex, size_t offset, size_t stride) -> void {

    if (InputLayout.has_value()) {
        glVertexArrayVertexBuffer(*InputLayout, bindingIndex, buffer, offset, stride);
    }
}

auto TGraphicsPipeline::DrawArrays(int32_t vertexOffset, size_t vertexCount) -> void {

    glDrawArrays(PrimitiveTopology, vertexOffset, vertexCount);
}

auto TGraphicsPipeline::DrawElements(uint32_t indexBuffer, size_t elementCount) -> void {

    if (g_lastIndexBuffer != indexBuffer) {
        glVertexArrayElementBuffer(InputLayout.has_value() ? InputLayout.value() : g_defaultInputLayout, indexBuffer);
        g_lastIndexBuffer = indexBuffer;
    }

    glDrawElements(PrimitiveTopology, elementCount, GL_UNSIGNED_INT, nullptr);
}

auto TGraphicsPipeline::DrawElementsInstanced(uint32_t indexBuffer, size_t elementCount, size_t instanceCount) -> void {

    if (g_lastIndexBuffer != indexBuffer) {
        glVertexArrayElementBuffer(InputLayout.has_value() ? InputLayout.value() : g_defaultInputLayout, indexBuffer);
        g_lastIndexBuffer = indexBuffer;
    }

    glDrawElementsInstanced(PrimitiveTopology, elementCount, GL_UNSIGNED_INT, nullptr, instanceCount);
}

// - Engine -------------------------------------------------------------------

struct TGpuVertexPosition {
    glm::vec3 Position;
};

struct TGpuPackedVertexNormalTangentUvTangentSign {
    uint32_t Normal;
    uint32_t Tangent;
    glm::vec4 UvAndTangentSign;
};

struct TGpuVertexPositionColor {
    glm::vec3 Position;
    glm::vec4 Color;
};

struct TGpuGlobalUniforms {

    glm::mat4 ProjectionMatrix;
    glm::mat4 ViewMatrix;
    glm::vec4 CameraPosition;
};

struct TGpuGlobalLight {
    glm::mat4 ProjectionMatrix;
    glm::mat4 ViewMatrix;
    glm::vec4 Direction;
    glm::vec4 Strength;
};

struct TGpuDebugLine {
    glm::vec3 StartPosition;
    glm::vec4 StartColor;
    glm::vec3 EndPosition;
    glm::vec4 EndColor;
};

struct TGpuMesh {
    std::string_view Name;
    uint32_t VertexPositionBuffer;
    uint32_t VertexNormalUvTangentBuffer;
    uint32_t IndexBuffer;

    size_t VertexCount;
    size_t IndexCount;

    glm::mat4 InitialTransform;
};

struct TCpuMaterial {
    glm::vec4 BaseColor;
    glm::vec4 Factors; // use .x = basecolor factor .y = normal strength, .z = metalness, .w = roughness

    uint32_t BaseColorTextureId;
    uint32_t BaseColorTextureSamplerId;
    uint32_t NormalTextureId;
    uint32_t NormalTextureSamplerId;

    uint32_t ArmTextureId;
    uint32_t ArmTextureSamplerId;
    uint32_t EmissiveTextureId;
    uint32_t EmissiveTextureSamplerId;
};

struct TGpuMaterial {
    glm::vec4 BaseColor;
    glm::vec4 Factors; // use .x = basecolor factor .y = normal strength, .z = metalness, .w = roughness

    uint64_t BaseColorTexture;
    uint64_t NormalTexture;

    uint64_t ArmTexture;
    uint64_t EmissiveTexture;

    uint32_t BaseColorTextureId;
    uint32_t NormalTextureId;
};

struct TCpuGlobalLight {

    float Azimuth;
    float Elevation;
    glm::vec3 Color;
    float Strength;

    int32_t ShadowMapSize;
    float ShadowMapNear;
    float ShadowMapFar;

    bool ShowFrustum;
};

struct TGpuObject {
    glm::mat4x4 WorldMatrix;
    glm::ivec4 InstanceParameter;
};

struct TMeshInstance {
    glm::mat4 WorldMatrix;
};

struct TCamera {

    glm::vec3 Position = {0.0f, 0.0f, 5.0f};
    float Pitch = {};
    float Yaw = {glm::radians(-90.0f)}; // look at 0, 0, -1

    auto GetForwardDirection() -> const glm::vec3
    {
        return glm::vec3{cos(Pitch) * cos(Yaw), sin(Pitch), cos(Pitch) * sin(Yaw)};
    }

    auto GetViewMatrix() -> const glm::mat4
    {
        return glm::lookAt(Position, Position + GetForwardDirection(), glm::vec3(0, 1, 0));
    }
};

// - Io -----------------------------------------------------------------------

auto ReadBinaryFromFile(const std::filesystem::path& filePath) -> std::pair<std::unique_ptr<std::byte[]>, std::size_t> {
    std::size_t fileSize = std::filesystem::file_size(filePath);
    auto memory = std::make_unique<std::byte[]>(fileSize);
    std::ifstream file{filePath, std::ifstream::binary};
    std::copy(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), reinterpret_cast<char*>(memory.get()));
    return {std::move(memory), fileSize};
}

// - Assets -------------------------------------------------------------------

using TAssetMeshId = TId<struct AssetMeshIdMarker>;
using TAssetImageId = TId<struct AssetImageIdMarker>;
using TAssetTextureId = TId<struct AssetTextureIdMarker>;
using TAssetSamplerId = TId<struct AssetSamplerIdMarker>;
using TAssetMaterialId = TId<struct AssetMaterialIdMarker>;

enum class TMaterialChannelUsage {
    SRgb, // use sRGB Format
    Normal, // use RG-format or bc7 when compressed
    MetalnessRoughness // single channelism or also RG?
};

struct TAssetMesh {
    std::string_view Name;
    glm::mat4 InitialTransform;
    std::vector<TGpuVertexPosition> VertexPositions;
    std::vector<TGpuPackedVertexNormalTangentUvTangentSign> VertexNormalUvTangents;
    std::vector<uint32_t> Indices;
    std::string MaterialName;
};

struct TAssetSampler {
    TTextureAddressMode AddressModeU = TTextureAddressMode::ClampToEdge;
    TTextureAddressMode AddressModeV = TTextureAddressMode::ClampToEdge;
    TTextureMagFilter MagFilter = TTextureMagFilter::Linear;
    TTextureMinFilter MinFilter = TTextureMinFilter::Linear;
};

struct TAssetImage {

    int32_t Width = 0;
    int32_t Height = 0;
    int32_t PixelType = 0;
    int32_t Bits = 8;
    int32_t Components = 0;
    std::string Name;

    std::unique_ptr<std::byte[]> EncodedData = {};
    std::size_t EncodedDataSize = 0;

    std::unique_ptr<unsigned char[]> DecodedData = {};

    size_t Index = 0;
};

struct TAssetTexture {
    std::string ImageName;
    std::string SamplerName;
};

struct TAssetMaterialChannel {
    TMaterialChannelUsage Usage;
    std::string Image;
    std::string Sampler; 
};

struct TAssetMaterial {
    glm::vec4 BaseColorFactor;
    float MetallicFactor;
    glm::vec3 EmissiveFactor;
    float EmissiveStrength;
    float NormalStrength;
    std::optional<TAssetMaterialChannel> BaseColorChannel;
    std::optional<TAssetMaterialChannel> NormalsChannel;
    std::optional<TAssetMaterialChannel> OcclusionRoughnessMetallnessChannel;
    std::optional<TAssetMaterialChannel> EmissiveChannel;
};

std::unordered_map<std::string, TAssetImage> g_assetImages = {};
std::unordered_map<std::string, TAssetSampler> g_assetSamplers = {};
std::unordered_map<std::string, TAssetTexture> g_assetTextures = {};
std::unordered_map<std::string, TAssetMaterial> g_assetMaterials = {};
std::unordered_map<std::string, TAssetMesh> g_assetMeshes = {};
std::unordered_map<std::string, std::vector<std::string>> g_assetModelMeshes = {};

auto GetSafeResourceName(
    const char* const baseName,
    const char* const text,
    const char* const resourceType,
    const std::size_t resourceIndex) -> std::string {

    return (text == nullptr) || strlen(text) == 0
        ? std::format("{}-{}-{}", baseName, resourceType, resourceIndex)
        : std::format("{}-{}", baseName, text);
}

auto AddAssetMesh(
    const std::string& assetMeshName,
    const TAssetMesh& assetMesh) -> void {

    assert(!assetMeshName.empty());
    PROFILER_ZONESCOPEDN("AddAssetMesh");

    g_assetMeshes[assetMeshName] = assetMesh;
}

auto AddAssetImage(
    const std::string& assetImageName,
    TAssetImage&& assetImage) -> void {

    assert(!assetImageName.empty());
    PROFILER_ZONESCOPEDN("AddAssetImage");

    g_assetImages[assetImageName] = std::move(assetImage);
}

auto AddAssetMaterial(
    const std::string& assetMaterialName,
    const TAssetMaterial& assetMaterial) -> void {

    assert(!assetMaterialName.empty());
    PROFILER_ZONESCOPEDN("AddAssetMaterial");

    g_assetMaterials[assetMaterialName] = assetMaterial;
}

auto AddAssetTexture(
    const std::string& assetTextureName,
    const TAssetTexture& assetTexture) -> void {

    assert(!assetTextureName.empty());
    PROFILER_ZONESCOPEDN("AddAssetTexture");

    g_assetTextures[assetTextureName] = assetTexture;
}

auto AddAssetSampler(
    const std::string& assetSamplerName,
    const TAssetSampler& assetSampler) -> void {

    assert(!assetSamplerName.empty());
    PROFILER_ZONESCOPEDN("AddAssetSampler");

    g_assetSamplers[assetSamplerName] = assetSampler;
}

auto GetAssetImage(const std::string& assetImageName) -> TAssetImage& {

    assert(!assetImageName.empty() || g_assetImages.contains(assetImageName));
    PROFILER_ZONESCOPEDN("GetAssetImage");

    return g_assetImages.at(assetImageName);
}

auto GetAssetMesh(const std::string& assetMeshName) -> TAssetMesh& {

    assert(!assetMeshName.empty() || g_assetMeshes.contains(assetMeshName));
    PROFILER_ZONESCOPEDN("GetAssetMesh");

    return g_assetMeshes.at(assetMeshName);
}

auto GetAssetMaterial(const std::string& assetMaterialName) -> TAssetMaterial& {

    assert(!assetMaterialName.empty() || g_assetMaterials.contains(assetMaterialName));
    PROFILER_ZONESCOPEDN("GetAssetMaterial");

    return g_assetMaterials.at(assetMaterialName);
}

auto GetAssetModelMeshNames(const std::string& modelName) -> std::vector<std::string>& {

    assert(!modelName.empty() || g_assetModelMeshes.contains(modelName));
    PROFILER_ZONESCOPEDN("GetAssetModelMeshNames");

    return g_assetModelMeshes.at(modelName);
}

auto GetAssetSampler(const std::string& assetSamplerName) -> TAssetSampler& {

    assert(!assetSamplerName.empty() || g_assetSamplers.contains(assetSamplerName));
    PROFILER_ZONESCOPEDN("GetAssetSampler");

    return g_assetSamplers.at(assetSamplerName);
}

/*
auto GetAssetSampler(const SAssetSamplerId& assetSamplerId) -> TSamplerDescriptor {

    PROFILER_ZONESCOPEDN("GetAssetSampler");
    for (const auto& [key, value] : g_assetSamplers) {
        if (value == assetSamplerId) {
            return key;
        }
    }

    spdlog::error("Unable to find Asset Sampler");
    return TSamplerDescriptor {};
}
*/

auto GetAssetTexture(const std::string& assetTextureName) -> TAssetTexture& {

    PROFILER_ZONESCOPEDN("GetAssetTexture");
    assert(!assetTextureName.empty());

    return g_assetTextures[assetTextureName];
}

auto GetLocalTransform(const fastgltf::Node& node) -> glm::mat4 {

    glm::mat4 transform{1.0};

    if (auto* trs = std::get_if<fastgltf::TRS>(&node.transform))
    {
        auto rotation = glm::quat{trs->rotation[3], trs->rotation[0], trs->rotation[1], trs->rotation[2]};
        auto scale = glm::make_vec3(trs->scale.data());
        auto translation = glm::make_vec3(trs->translation.data());

        auto rotationMatrix = glm::mat4_cast(rotation);

        // T * R * S
        transform = glm::scale(glm::translate(glm::mat4(1.0f), translation) * rotationMatrix, scale);
    }
    else if (auto* mat = std::get_if<fastgltf::Node::TransformMatrix>(&node.transform))
    {
        const auto& m = *mat;
        transform = glm::make_mat4x4(m.data());
    }

    return transform;
}

auto SignNotZero(glm::vec2 v) -> glm::vec2 {

    return glm::vec2((v.x >= 0.0f) ? +1.0f : -1.0f, (v.y >= 0.0f) ? +1.0f : -1.0f);
}

auto EncodeNormal(glm::vec3 normal) -> glm::vec2 {

    glm::vec2 encodedNormal = glm::vec2{normal.x, normal.y} * (1.0f / (abs(normal.x) + abs(normal.y) + abs(normal.z)));
    return (normal.z <= 0.0f)
        ? ((1.0f - glm::abs(glm::vec2{encodedNormal.y, encodedNormal.x})) * SignNotZero(encodedNormal))
        : encodedNormal;
}

auto GetVertices(
    const fastgltf::Asset& asset, 
    const fastgltf::Primitive& primitive) -> std::pair<std::vector<TGpuVertexPosition>, std::vector<TGpuPackedVertexNormalTangentUvTangentSign>> {

    PROFILER_ZONESCOPEDN("GetVertices");

    std::vector<glm::vec3> positions;
    auto& positionAccessor = asset.accessors[primitive.findAttribute("POSITION")->second];
    positions.resize(positionAccessor.count);
    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,
                                                  positionAccessor,
                                                  [&](glm::vec3 position, std::size_t index) { positions[index] = position; });

    std::vector<glm::vec3> normals;
    auto& normalAccessor = asset.accessors[primitive.findAttribute("NORMAL")->second];
    normals.resize(normalAccessor.count);
    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,
                                                  normalAccessor,
                                                  [&](glm::vec3 normal, std::size_t index) { normals[index] = normal; });

    std::vector<glm::vec4> tangents;
    auto& tangentAccessor = asset.accessors[primitive.findAttribute("TANGENT")->second];
    tangents.resize(tangentAccessor.count);
    fastgltf::iterateAccessorWithIndex<glm::vec4>(asset,
                                                  tangentAccessor,
                                                  [&](glm::vec4 tangent, std::size_t index) { tangents[index] = tangent; });

    std::vector<glm::vec3> uvs;
    if (primitive.findAttribute("TEXCOORD_0") != primitive.attributes.end())
    {
        auto& uvAccessor = asset.accessors[primitive.findAttribute("TEXCOORD_0")->second];
        uvs.resize(uvAccessor.count);
        fastgltf::iterateAccessorWithIndex<glm::vec2>(asset,
                                                      uvAccessor,
                                                      [&](glm::vec2 uv, std::size_t index) { uvs[index] = glm::vec3{uv, 0.0f}; });
    }
    else
    {
        uvs.resize(positions.size(), {});
    }

    std::vector<TGpuVertexPosition> verticesPosition;
    std::vector<TGpuPackedVertexNormalTangentUvTangentSign> verticesNormalUvTangent;
    verticesPosition.resize(positions.size());
    verticesNormalUvTangent.resize(positions.size());

    for (size_t i = 0; i < positions.size(); i++) {
        verticesPosition[i] = {
            positions[i],
        };
        uvs[i].z = tangents[i].w;
        verticesNormalUvTangent[i] = {
            glm::packSnorm2x16(EncodeNormal(normals[i])),
            glm::packSnorm2x16(EncodeNormal(tangents[i].xyz())),
            glm::vec4(uvs[i], 0.0f),
        };
    }

    return {verticesPosition, verticesNormalUvTangent};
}

auto GetIndices(
    const fastgltf::Asset& asset,
    const fastgltf::Primitive& primitive) -> std::vector<uint32_t> {

    PROFILER_ZONESCOPEDN("GetIndices");

    auto indices = std::vector<uint32_t>();
    auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
    indices.resize(accessor.count);
    fastgltf::iterateAccessorWithIndex<uint32_t>(asset, accessor, [&](uint32_t value, size_t index)
    {
        indices[index] = value;
    });
    return indices;
}

auto CreateAssetMesh(
    const std::string& name,
    glm::mat4 initialTransform,
    const std::pair<std::vector<TGpuVertexPosition>, std::vector<TGpuPackedVertexNormalTangentUvTangentSign>>& vertices,
    const std::vector<uint32_t> indices,
    const std::string& materialName) -> TAssetMesh {

    PROFILER_ZONESCOPEDN("CreateAssetMesh");

    return TAssetMesh{
        .Name = name,
        .InitialTransform = initialTransform,
        .VertexPositions = std::move(vertices.first),
        .VertexNormalUvTangents = std::move(vertices.second),
        .Indices = std::move(indices),
        .MaterialName = materialName
    };
}

auto CreateAssetImage(
    const void* data, 
    std::size_t dataSize, 
    fastgltf::MimeType mimeType,
    std::string_view name) -> TAssetImage {

    PROFILER_ZONESCOPEDN("CreateAssetImage");

    auto dataCopy = std::make_unique<std::byte[]>(dataSize);
    std::copy_n(static_cast<const std::byte*>(data), dataSize, dataCopy.get());

    return TAssetImage {
        .Name = std::string(name),
        .EncodedData = std::move(dataCopy),
        .EncodedDataSize = dataSize,
    };
}

auto CreateAssetMaterial(
    const std::string& modelName,
    const fastgltf::Asset& asset,
    const fastgltf::Material& fgMaterial,
    size_t assetMaterialIndex) -> void {

    PROFILER_ZONESCOPEDN("CreateAssetMaterial");

    TAssetMaterial assetMaterial = {};
    assetMaterial.BaseColorFactor = glm::make_vec4(fgMaterial.pbrData.baseColorFactor.data());
    assetMaterial.MetallicFactor = fgMaterial.pbrData.metallicFactor;
    assetMaterial.EmissiveFactor = glm::make_vec3(fgMaterial.emissiveFactor.data());
    assetMaterial.EmissiveStrength = fgMaterial.emissiveStrength;

    if (fgMaterial.pbrData.baseColorTexture.has_value()) {
        auto& fgBaseColorTexture = fgMaterial.pbrData.baseColorTexture.value();
        auto& fgTexture = asset.textures[fgBaseColorTexture.textureIndex];
        auto textureName = asset.images[fgTexture.imageIndex.value_or(0)].name.data();
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), textureName, "image", fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0));

        assetMaterial.BaseColorChannel = TAssetMaterialChannel{
            .Usage = TMaterialChannelUsage::SRgb,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }

    if (fgMaterial.normalTexture.has_value()) {
        auto& fgNormalTexture = fgMaterial.normalTexture.value();
        auto& fgTexture = asset.textures[fgNormalTexture.textureIndex];
        auto textureName = asset.images[fgTexture.imageIndex.value_or(0)].name.data();
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), textureName, "image", fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0));

        assetMaterial.BaseColorChannel = TAssetMaterialChannel{
            .Usage = TMaterialChannelUsage::Normal,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }

    if (fgMaterial.pbrData.metallicRoughnessTexture.has_value()) {
        auto& fgMetallicRoughnessTexture = fgMaterial.pbrData.metallicRoughnessTexture.value();
        auto& fgTexture = asset.textures[fgMetallicRoughnessTexture.textureIndex];
        auto textureName = asset.images[fgTexture.imageIndex.value_or(0)].name.data();
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), textureName, "image", fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0));

        assetMaterial.OcclusionRoughnessMetallnessChannel = TAssetMaterialChannel{
            .Usage = TMaterialChannelUsage::MetalnessRoughness,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }

    if (fgMaterial.emissiveTexture.has_value()) {
        auto& fgEmissiveTexture = fgMaterial.emissiveTexture.value();
        auto& fgTexture = asset.textures[fgEmissiveTexture.textureIndex];
        auto textureName = asset.images[fgTexture.imageIndex.value_or(0)].name.data();
        auto fgTextureImageName = GetSafeResourceName(modelName.data(), textureName, "image", fgTexture.imageIndex.value_or(0));
        auto fgTextureSamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0));

        assetMaterial.BaseColorChannel = TAssetMaterialChannel{
            .Usage = TMaterialChannelUsage::SRgb,
            .Image = fgTextureImageName,
            .Sampler = fgTextureSamplerName,
        };
    }
    
    auto materialName = GetSafeResourceName(modelName.data(), fgMaterial.name.data(), "material", assetMaterialIndex);

    return AddAssetMaterial(materialName, assetMaterial);
}

auto LoadModelFromFile(
    std::string modelName,
    std::filesystem::path filePath) -> void {

    PROFILER_ZONESCOPEDN("LoadModelFromFile");

    fastgltf::Parser parser(
        fastgltf::Extensions::KHR_mesh_quantization |
        fastgltf::Extensions::EXT_mesh_gpu_instancing);

    constexpr auto gltfOptions =
        fastgltf::Options::DontRequireValidAssetMember |
        fastgltf::Options::AllowDouble |
        fastgltf::Options::LoadGLBBuffers |
        fastgltf::Options::LoadExternalBuffers |
        fastgltf::Options::LoadExternalImages;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    auto assetResult = parser.loadGltf(&data, filePath.parent_path(), gltfOptions);
    if (assetResult.error() != fastgltf::Error::None)
    {
        spdlog::error("fastgltf: Failed to load glTF: {}", fastgltf::getErrorMessage(assetResult.error()));
        return;
    }

    auto& fgAsset = assetResult.get();

    // images

    auto assetImageIds = std::vector<TAssetImageId>(fgAsset.images.size());
    const auto assetImageIndices = std::ranges::iota_view{(size_t)0, fgAsset.images.size()};

    std::transform(
        poolstl::execution::par,
        assetImageIndices.begin(),
        assetImageIndices.end(),
        assetImageIds.begin(),
        [&](size_t imageIndex) {

        PROFILER_ZONESCOPEDN("Create Image");
        const auto& fgImage = fgAsset.images[imageIndex];
        auto imageData = [&] {
            if (const auto* filePathUri = std::get_if<fastgltf::sources::URI>(&fgImage.data)) {
                auto filePathFixed = std::filesystem::path(filePathUri->uri.path());
                auto filePathParent = filePath.parent_path();
                if (filePathFixed.is_relative()) {
                    filePathFixed = filePath.parent_path() / filePathFixed;
                }
                auto fileData = ReadBinaryFromFile(filePathFixed);
                return CreateAssetImage(fileData.first.get(), fileData.second, filePathUri->mimeType, fgImage.name);
            }
            if (const auto* array = std::get_if<fastgltf::sources::Array>(&fgImage.data)) {
                return CreateAssetImage(array->bytes.data(), array->bytes.size(), array->mimeType, fgImage.name);
            }
            if (const auto* vector = std::get_if<fastgltf::sources::Vector>(&fgImage.data)) {
                return CreateAssetImage(vector->bytes.data(), vector->bytes.size(), vector->mimeType, fgImage.name);
            }
            if (const auto* view = std::get_if<fastgltf::sources::BufferView>(&fgImage.data)) {
                auto& bufferView = fgAsset.bufferViews[view->bufferViewIndex];
                auto& buffer = fgAsset.buffers[bufferView.bufferIndex];
                if (const auto* array = std::get_if<fastgltf::sources::Array>(&buffer.data)) {
                    return CreateAssetImage(
                        array->bytes.data() + bufferView.byteOffset,
                        bufferView.byteLength,
                        view->mimeType,
                        fgImage.name);
                }
            }
            
            return TAssetImage{};
        }();

        auto assetImageId = imageIndex;

        int32_t width = 0;
        int32_t height = 0;
        int32_t components = 0;
        auto pixels = LoadImageFromMemory(
            imageData.EncodedData.get(),
            imageData.EncodedDataSize,
            &width,
            &height,
            &components);

        imageData.Width = width;
        imageData.Height = height;
        imageData.Components = 4; // LoadImageFromMemory is forcing 4 components
        imageData.DecodedData.reset(pixels);
        imageData.Index = assetImageId;

        AddAssetImage(GetSafeResourceName(modelName.data(), imageData.Name.data(), "image", imageIndex), std::move(imageData));
        return static_cast<TAssetImageId>(assetImageId);
    });

    // samplers

    auto samplerIds = std::vector<TAssetSamplerId>(fgAsset.samplers.size());
    const auto samplerIndices = std::ranges::iota_view{(size_t)0, fgAsset.samplers.size()};
    std::transform(poolstl::execution::par, samplerIndices.begin(), samplerIndices.end(), samplerIds.begin(), [&](size_t samplerIndex) {

        PROFILER_ZONESCOPEDN("Load Sampler");
        const fastgltf::Sampler& fgSampler = fgAsset.samplers[samplerIndex];

        const auto getAddressMode = [](const fastgltf::Wrap& wrap) {
            switch (wrap) {
                case fastgltf::Wrap::ClampToEdge: return TTextureAddressMode::ClampToEdge;
                case fastgltf::Wrap::MirroredRepeat: return TTextureAddressMode::RepeatMirrored;
                case fastgltf::Wrap::Repeat: return TTextureAddressMode::Repeat;
                default:
                    return TTextureAddressMode::ClampToEdge;
            }
        };

        //TODO(deccer) reinvent min/mag filterisms, this sucks here
        const auto getMinFilter = [](const fastgltf::Filter& minFilter) {
            switch (minFilter) {
                case fastgltf::Filter::Linear: return TTextureMinFilter::Linear;
                case fastgltf::Filter::LinearMipMapLinear: return TTextureMinFilter::LinearMipmapLinear;
                case fastgltf::Filter::LinearMipMapNearest: return TTextureMinFilter::LinearMipmapNearest;
                case fastgltf::Filter::Nearest: return TTextureMinFilter::Nearest;
                case fastgltf::Filter::NearestMipMapLinear: return TTextureMinFilter::NearestMipmapLinear;
                case fastgltf::Filter::NearestMipMapNearest: return TTextureMinFilter::NearestMipmapNearest;
                default: std::unreachable();
            }
        };

        const auto getMagFilter = [](const fastgltf::Filter& magFilter) {
            switch (magFilter) {
                case fastgltf::Filter::Linear: return TTextureMagFilter::Linear;
                case fastgltf::Filter::Nearest: return TTextureMagFilter::Nearest;
                default: std::unreachable();
            }
        };

        auto assetSampler = TAssetSampler {
            .AddressModeU = getAddressMode(fgSampler.wrapS),
            .AddressModeV = getAddressMode(fgSampler.wrapT),
            .MagFilter = getMagFilter(fgSampler.magFilter.has_value() ? *fgSampler.magFilter : fastgltf::Filter::Nearest),
            .MinFilter = getMinFilter(fgSampler.minFilter.has_value() ? *fgSampler.minFilter : fastgltf::Filter::Nearest),
        };

        auto assetSamplerId = g_assetSamplers.size() + samplerIndex;
        AddAssetSampler(GetSafeResourceName(modelName.data(), fgSampler.name.data(), "sampler", assetSamplerId), assetSampler);
        return static_cast<TAssetSamplerId>(assetSamplerId);
    });

    // textures

    auto assetTextures = std::vector<TAssetTextureId>(fgAsset.textures.size());
    const auto assetTextureIndices = std::ranges::iota_view{(size_t)0, fgAsset.textures.size()};
    for (auto assetTextureIndex : assetTextureIndices) {

        auto& fgTexture = fgAsset.textures[assetTextureIndex];
        auto textureName = GetSafeResourceName(modelName.data(), fgTexture.name.data(), "texture", assetTextureIndex);

        AddAssetTexture(textureName, TAssetTexture{
            .ImageName = GetSafeResourceName(modelName.data(), nullptr, "image", fgTexture.imageIndex.value_or(0)),
            .SamplerName = GetSafeResourceName(modelName.data(), nullptr, "sampler", fgTexture.samplerIndex.value_or(0))
        });
    }

    // materials

    auto assetMaterialIds = std::vector<TAssetMaterialId>(fgAsset.materials.size());
    const auto assetMaterialIndices = std::ranges::iota_view{(size_t)0, fgAsset.materials.size()};
    for (auto assetMaterialIndex : assetMaterialIndices) {

        auto& fgMaterial = fgAsset.materials[assetMaterialIndex];

        CreateAssetMaterial(modelName, fgAsset, fgMaterial, assetMaterialIndex);
    }

    std::stack<std::pair<const fastgltf::Node*, glm::mat4>> nodeStack;
    glm::mat4 rootTransform = glm::mat4(1.0f);

    for (auto nodeIndex : fgAsset.scenes[0].nodeIndices)
    {
        nodeStack.emplace(&fgAsset.nodes[nodeIndex], rootTransform);
    }
   
    while (!nodeStack.empty())
    {
        PROFILER_ZONESCOPEDN("Load Primitive");

        decltype(nodeStack)::value_type top = nodeStack.top();
        const auto& [fgNode, parentGlobalTransform] = top;
        nodeStack.pop();

        glm::mat4 localTransform = GetLocalTransform(*fgNode);
        glm::mat4 globalTransform = parentGlobalTransform * localTransform;

        for (auto childNodeIndex : fgNode->children)
        {
            nodeStack.emplace(&fgAsset.nodes[childNodeIndex], globalTransform);
        }

        if (!fgNode->meshIndex.has_value()) {
            continue;
        }

        auto meshIndex = fgNode->meshIndex.value();
        auto& fgMesh = fgAsset.meshes[meshIndex];

        for (auto primitiveIndex = 0; const auto& fgPrimitive : fgMesh.primitives)
        {
            const auto primitiveMaterialIndex = fgPrimitive.materialIndex.value_or(0);

            auto vertices = GetVertices(fgAsset, fgPrimitive);
            auto indices = GetIndices(fgAsset, fgPrimitive);

            auto meshName = GetSafeResourceName(modelName.data(), fgMesh.name.data(), "mesh", primitiveIndex);

            auto materialIndex = fgPrimitive.materialIndex.value_or(0);
            auto& fgMaterial = fgAsset.materials[materialIndex];
            auto materialName = GetSafeResourceName(modelName.data(), fgMaterial.name.data(), "material", materialIndex);

            auto assetMesh = CreateAssetMesh(meshName, globalTransform, std::move(vertices), std::move(indices), materialName);
            AddAssetMesh(meshName, assetMesh);
            g_assetModelMeshes[modelName].push_back(meshName);

            primitiveIndex++;
        }
    }
}

// - Renderer -----------------------------------------------------------------

using TGpuMeshId = TId<struct GpuMeshId>;
using TGpuSamplerId = TId<struct GpuSamplerId>;
using TGpuMaterialId = TId<struct GpuMaterialId>;

TIdGenerator<TGpuMeshId> g_gpuMeshCounter = {};

TFramebuffer g_depthPrePassFramebuffer = {};
TGraphicsPipeline g_depthPrePassGraphicsPipeline = {};

TFramebuffer g_geometryFramebuffer = {};
TGraphicsPipeline g_geometryGraphicsPipeline = {};

TFramebuffer g_resolveGeometryFramebuffer = {};
TGraphicsPipeline g_resolveGeometryGraphicsPipeline = {};

std::vector<TGpuGlobalLight> g_gpuGlobalLights;

bool g_drawDebugLines = true;
std::vector<TGpuDebugLine> g_debugLines;
uint32_t g_debugInputLayout = 0;
uint32_t g_debugLinesVertexBuffer = 0;
TGraphicsPipeline g_debugLinesGraphicsPipeline = {};

TFramebuffer g_fxaaFramebuffer = {};
TGraphicsPipeline g_fxaaGraphicsPipeline = {};

TGraphicsPipeline g_fstGraphicsPipeline = {};
TSamplerId g_fstSamplerNearestClampToEdge = {};

std::unordered_map<std::string, TGpuMesh> g_gpuMeshes = {};
std::unordered_map<std::string, TSampler> g_gpuSamplers = {};
std::unordered_map<std::string, TCpuMaterial> g_cpuMaterials = {};
std::unordered_map<std::string, TGpuMaterial> g_gpuMaterials = {};

auto DrawFullscreenTriangleWithTexture(const TTexture& texture) -> void {

    g_fstGraphicsPipeline.Bind();
    auto& sampler = g_samplers[size_t(g_fstSamplerNearestClampToEdge)];
    g_fstGraphicsPipeline.BindTextureAndSampler(0, texture.Id, sampler.Id);
    g_fstGraphicsPipeline.DrawArrays(0, 3);
}

auto CreateGpuMesh(const std::string& assetMeshName) -> void {

    auto& assetMesh = GetAssetMesh(assetMeshName);

    uint32_t buffers[3] = {};
    {
        PROFILER_ZONESCOPEDN("Create GL Buffers + Upload Data");

        glCreateBuffers(3, buffers);
        SetDebugLabel(buffers[0], GL_BUFFER, std::format("{}_position", assetMeshName));
        SetDebugLabel(buffers[1], GL_BUFFER, std::format("{}_normal_uv_tangent", assetMeshName));
        SetDebugLabel(buffers[2], GL_BUFFER, std::format("{}_indices", assetMeshName));
        glNamedBufferStorage(buffers[0], sizeof(TGpuVertexPosition) * assetMesh.VertexPositions.size(),
                             assetMesh.VertexPositions.data(), 0);
        glNamedBufferStorage(buffers[1], sizeof(TGpuPackedVertexNormalTangentUvTangentSign) * assetMesh.VertexNormalUvTangents.size(),
                             assetMesh.VertexNormalUvTangents.data(), 0);
        glNamedBufferStorage(buffers[2], sizeof(uint32_t) * assetMesh.Indices.size(), assetMesh.Indices.data(), 0);
    }

    auto gpuMesh = TGpuMesh{
        .Name = assetMeshName,
        .VertexPositionBuffer = buffers[0],
        .VertexNormalUvTangentBuffer = buffers[1],
        .IndexBuffer = buffers[2],

        .VertexCount = assetMesh.VertexPositions.size(),
        .IndexCount = assetMesh.Indices.size(),

        .InitialTransform = assetMesh.InitialTransform,
    };

    {
        PROFILER_ZONESCOPEDN("Add gpu mesh");
        g_gpuMeshes[assetMeshName] = gpuMesh;
    }
}

auto GetGpuMesh(const std::string& assetMeshName) -> TGpuMesh& {
    assert(!assetMeshName.empty() && g_gpuMeshes.contains(assetMeshName));

    return g_gpuMeshes[assetMeshName];
}

auto GetCpuMaterial(const std::string& assetMaterialName) -> TCpuMaterial& {
    assert(!assetMaterialName.empty() && g_cpuMaterials.contains(assetMaterialName));

    return g_cpuMaterials[assetMaterialName];
}

auto GetGpuMaterial(const std::string& assetMaterialName) -> TGpuMaterial& {
    assert(!assetMaterialName.empty() && g_gpuMaterials.contains(assetMaterialName));

    return g_gpuMaterials[assetMaterialName];
}

auto CreateResidentTextureForMaterialChannel(TAssetMaterialChannel& assetMaterialChannel) -> int64_t {

    PROFILER_ZONESCOPEDN("CreateResidentTextureForMaterialChannel");

    auto& image = GetAssetImage(assetMaterialChannel.Image);

    auto textureId = CreateTexture(TCreateTextureDescriptor{
        .TextureType = TTextureType::Texture2D,
        .Format = TFormat::R8G8B8A8_UNORM,
        .Extent = TExtent3D{ static_cast<uint32_t>(image.Width), static_cast<uint32_t>(image.Height), 1u},
        .MipMapLevels = static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(image.Width), static_cast<float>(image.Height))))),
        .Layers = 1,
        .SampleCount = TSampleCount::One,
        .Label = image.Name,
    });
    
    UploadTexture(textureId, TUploadTextureDescriptor{
        .Level = 0,
        .Offset = TOffset3D{ 0, 0, 0},
        .Extent = TExtent3D{ static_cast<uint32_t>(image.Width), static_cast<uint32_t>(image.Height), 1u},
        .UploadFormat = TUploadFormat::Auto,
        .UploadType = TUploadType::Auto,
        .PixelData = image.DecodedData.get()
    });

    GenerateMipmaps(textureId);

    //auto& sampler = GetAssetSampler(assetMaterialChannel.Sampler);

    return MakeTextureResident(textureId);
}

auto CreateTextureForMaterialChannel(TAssetMaterialChannel& assetMaterialChannel) -> uint32_t {

    PROFILER_ZONESCOPEDN("CreateTextureForMaterialChannel");

    auto& image = GetAssetImage(assetMaterialChannel.Image);

    auto textureId = CreateTexture(TCreateTextureDescriptor{
        .TextureType = TTextureType::Texture2D,
        .Format = TFormat::R8G8B8A8_UNORM,
        .Extent = TExtent3D{ static_cast<uint32_t>(image.Width), static_cast<uint32_t>(image.Height), 1u},
        .MipMapLevels = static_cast<uint32_t>(glm::floor(glm::log2(glm::max(static_cast<float>(image.Width), static_cast<float>(image.Height))))),
        .Layers = 1,
        .SampleCount = TSampleCount::One,
        .Label = image.Name,
    });

    UploadTexture(textureId, TUploadTextureDescriptor{
        .Level = 0,
        .Offset = TOffset3D{ 0, 0, 0},
        .Extent = TExtent3D{ static_cast<uint32_t>(image.Width), static_cast<uint32_t>(image.Height), 1u},
        .UploadFormat = TUploadFormat::Auto,
        .UploadType = TUploadType::Auto,
        .PixelData = image.DecodedData.get()
    });

    GenerateMipmaps(textureId);

    //auto& sampler = GetAssetSampler(assetMaterialChannel.Sampler);

    return GetTexture(textureId).Id;
}

auto CreateGpuMaterial(const std::string& assetMaterialName) -> void {

    PROFILER_ZONESCOPEDN("CreateGpuMaterial");

    auto& assetMaterial = GetAssetMaterial(assetMaterialName);

    uint64_t baseColorTextureHandle = assetMaterial.BaseColorChannel.has_value()
        ? CreateResidentTextureForMaterialChannel(assetMaterial.BaseColorChannel.value())
        : 0;

    uint64_t normalTextureHandle = assetMaterial.NormalsChannel.has_value()
        ? CreateResidentTextureForMaterialChannel(assetMaterial.NormalsChannel.value())
        : 0;

    auto gpuMaterial = TGpuMaterial{
        .BaseColor = assetMaterial.BaseColorFactor,
        .BaseColorTexture = baseColorTextureHandle,
        .NormalTexture = normalTextureHandle
    };

    g_gpuMaterials[assetMaterialName] = gpuMaterial;
}

auto CreateSamplerDescriptor(const TAssetSampler& assetSampler) -> TSamplerDescriptor {

    return TSamplerDescriptor{
        .AddressModeU = assetSampler.AddressModeU,
        .AddressModeV = assetSampler.AddressModeV,
        .MagFilter = assetSampler.MagFilter,
        .MinFilter = assetSampler.MinFilter,
    };
}

auto CreateCpuMaterial(const std::string& assetMaterialName) -> void {

    PROFILER_ZONESCOPEDN("CreateCpuMaterial");

    auto& assetMaterial = GetAssetMaterial(assetMaterialName);

    auto cpuMaterial = TCpuMaterial{
        .BaseColor = assetMaterial.BaseColorFactor,
    };

    auto& baseColorChannel = assetMaterial.BaseColorChannel;
    if (baseColorChannel.has_value()) {
        auto& baseColor = *baseColorChannel;
        auto& baseColorSampler = GetAssetSampler(baseColor.Sampler);

        cpuMaterial.BaseColorTextureId = CreateTextureForMaterialChannel(baseColor);
        auto samplerId = GetOrCreateSampler(CreateSamplerDescriptor(baseColorSampler));
        auto& sampler = g_samplers[size_t(samplerId)];
        cpuMaterial.BaseColorTextureSamplerId = sampler.Id;
    }

    auto& normalTextureChannel = assetMaterial.NormalsChannel;
    if (normalTextureChannel.has_value()) {
        auto& normalTexture = *normalTextureChannel;
        auto& normalTextureSampler = GetAssetSampler(normalTexture.Sampler);

        cpuMaterial.NormalTextureId = CreateTextureForMaterialChannel(normalTexture);
        auto samplerId = GetOrCreateSampler(CreateSamplerDescriptor(normalTextureSampler));
        auto& sampler = g_samplers[size_t(samplerId)];
        cpuMaterial.NormalTextureSamplerId = sampler.Id;
    }

    g_cpuMaterials[assetMaterialName] = cpuMaterial;
}

// - Game ---------------------------------------------------------------------

TCamera g_mainCamera = {};
float g_cameraSpeed = 4.0f;

std::vector<TCpuGlobalLight> g_globalLights;
bool g_globalLightsWasModified = true;

entt::registry g_gameRegistry = {};

// - Scene --------------------------------------------------------------------

struct TComponentTransform : public glm::mat4x4
{
    using glm::mat4x4::mat4x4;
    using glm::mat4x4::operator=;
    TComponentTransform() = default;
    TComponentTransform(glm::mat4x4 const& g) : glm::mat4x4(g) {}
    TComponentTransform(glm::mat4x4&& g) : glm::mat4x4(std::move(g)) {}
};

struct TComponentParent {
    std::vector<entt::entity> Children;
};

struct TComponentChildOf {
    entt::entity Parent;
};

struct TComponentMesh {
    std::string Mesh;
};

struct TComponentMaterial {
    std::string Material;
};

struct TComponentCreateGpuResourcesNecessary {
};

struct TComponentGpuMesh {
    std::string GpuMesh;
};

struct TComponentGpuMaterial {
    std::string GpuMaterial;
};

auto AddEntity(
    std::optional<entt::entity> parent,
    const std::string& assetMeshName,
    const std::string& assetMaterialName,
    glm::mat4x4 initialTransform) -> entt::entity {

    PROFILER_ZONESCOPEDN("AddEntity");

    auto entityId = g_gameRegistry.create();
    if (parent.has_value()) {
        auto parentComponent = g_gameRegistry.get_or_emplace<TComponentParent>(parent.value());
        parentComponent.Children.push_back(entityId);
        g_gameRegistry.emplace<TComponentChildOf>(entityId, parent.value());
    }
    g_gameRegistry.emplace<TComponentMesh>(entityId, assetMeshName);
    g_gameRegistry.emplace<TComponentMaterial>(entityId, assetMaterialName);
    g_gameRegistry.emplace<TComponentTransform>(entityId, initialTransform);
    g_gameRegistry.emplace<TComponentCreateGpuResourcesNecessary>(entityId);

    return entityId;
}

auto AddEntity(
    std::optional<entt::entity> parent,
    const std::string& modelName,
    glm::mat4 initialTransform) -> entt::entity {

    PROFILER_ZONESCOPEDN("AddEntity");

    auto entityId = g_gameRegistry.create();
    if (parent.has_value()) {
        auto& parentComponent = g_gameRegistry.get_or_emplace<TComponentParent>(parent.value());
        parentComponent.Children.push_back(entityId);

        g_gameRegistry.emplace<TComponentChildOf>(entityId, parent.value());
    }

    if (g_assetModelMeshes.contains(modelName)) {
        auto& modelMeshesNames = g_assetModelMeshes[modelName];
        for(auto& modelMeshName : modelMeshesNames) {

            auto& assetMesh = GetAssetMesh(modelMeshName);
            
            AddEntity(entityId, modelMeshName, assetMesh.MaterialName, initialTransform);
        }
    }

    return entityId;
}

// - Application --------------------------------------------------------------

const glm::vec3 g_unitX = glm::vec3{1.0f, 0.0f, 0.0f};
const glm::vec3 g_unitY = glm::vec3{0.0f, 1.0f, 0.0f};
const glm::vec3 g_unitZ = glm::vec3{0.0f, 0.0f, 1.0f};

constexpr ImVec2 g_imvec2UnitX = ImVec2(1, 0);
constexpr ImVec2 g_imvec2UnitY = ImVec2(0, 1);

GLFWwindow* g_window = nullptr;
ImGuiContext* g_imguiContext = nullptr;

bool g_isEditor = false;
bool g_sleepWhenWindowHasNoFocus = true;
bool g_windowHasFocus = false;
bool g_cursorJustEntered = false;
bool g_cursorIsActive = true;
float g_cursorSensitivity = 0.0025f;

glm::dvec2 g_cursorPosition = {};
glm::dvec2 g_cursorFrameOffset = {};

glm::ivec2 g_windowFramebufferSize = {};
glm::ivec2 g_windowFramebufferScaledSize = {};
bool g_windowFramebufferResized = false;

glm::ivec2 g_sceneViewerSize = {};
glm::ivec2 g_sceneViewerScaledSize = {};
bool g_sceneViewerResized = false;

bool g_isFxaaEnabled = false;

// - Implementation -----------------------------------------------------------

auto OnWindowKey(
    [[maybe_unused]] GLFWwindow* window,
    const int32_t key,
    [[maybe_unused]] int32_t scancode,
    const int32_t action,
    [[maybe_unused]] int32_t mods) -> void {

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        g_isEditor = !g_isEditor;
        g_windowFramebufferResized = !g_isEditor;
        g_sceneViewerResized = g_isEditor;
    }
}

auto OnWindowCursorEntered(
    [[maybe_unused]] GLFWwindow* window,
    int entered) -> void {

    if (entered) {
        g_cursorJustEntered = true;
    }

    g_windowHasFocus = entered == GLFW_TRUE;
}

auto OnWindowCursorPosition(
    [[maybe_unused]] GLFWwindow* window,
    double cursorPositionX,
    double cursorPositionY) -> void {

    if (g_cursorJustEntered)
    {
        g_cursorPosition = {cursorPositionX, cursorPositionY};
        g_cursorJustEntered = false;
    }        

    g_cursorFrameOffset += glm::dvec2{
        cursorPositionX - g_cursorPosition.x, 
        g_cursorPosition.y - cursorPositionY};
    g_cursorPosition = {cursorPositionX, cursorPositionY};
}

auto OnWindowFramebufferSizeChanged(
    [[maybe_unused]] GLFWwindow* window,
    const int width,
    const int height) -> void {

    g_windowFramebufferSize = glm::ivec2{width, height};
    g_windowFramebufferResized = true;
}

auto OnOpenGLDebugMessage(
    [[maybe_unused]] uint32_t source,
    uint32_t type,
    [[maybe_unused]] uint32_t id,
    [[maybe_unused]] uint32_t severity, 
    [[maybe_unused]] int32_t length,
    const char* message,
    [[maybe_unused]] const void* userParam) -> void {

    if (type == GL_DEBUG_TYPE_ERROR) {
        spdlog::error(message);
        debug_break();
    }
}

auto HandleCamera(float deltaTime) -> void {

    g_cursorIsActive = glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_2) == GLFW_RELEASE;
    glfwSetInputMode(g_window, GLFW_CURSOR, g_cursorIsActive ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

    if (!g_cursorIsActive) {
        glfwSetCursorPos(g_window, 0, 0);
        g_cursorPosition.x = 0;
        g_cursorPosition.y = 0;
    }

    const auto forward = g_mainCamera.GetForwardDirection();
    const auto right = glm::normalize(glm::cross(forward, g_unitY));

    auto tempCameraSpeed = g_cameraSpeed;
    if (glfwGetKey(g_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        tempCameraSpeed *= 4.0f;
    }
    if (glfwGetKey(g_window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
        tempCameraSpeed *= 40.0f;
    }
    if (glfwGetKey(g_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        tempCameraSpeed *= 0.25f;
    }
    if (glfwGetKey(g_window, GLFW_KEY_W) == GLFW_PRESS) {
        g_mainCamera.Position += forward * deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_S) == GLFW_PRESS) {
        g_mainCamera.Position -= forward * deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS) {
        g_mainCamera.Position += right * deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS) {
        g_mainCamera.Position -= right * deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_Q) == GLFW_PRESS) {
        g_mainCamera.Position.y -= deltaTime * tempCameraSpeed;
    }
    if (glfwGetKey(g_window, GLFW_KEY_E) == GLFW_PRESS) {
        g_mainCamera.Position.y += deltaTime * tempCameraSpeed;
    }

    if (!g_cursorIsActive) {

        g_mainCamera.Yaw += static_cast<float>(g_cursorFrameOffset.x * g_cursorSensitivity);
        g_mainCamera.Pitch += static_cast<float>(g_cursorFrameOffset.y * g_cursorSensitivity);
        g_mainCamera.Pitch = glm::clamp(g_mainCamera.Pitch, -glm::half_pi<float>() + 1e-4f, glm::half_pi<float>() - 1e-4f);    
    }

    g_cursorFrameOffset = {0.0, 0.0};
}

auto DeleteRendererFramebuffers() -> void {

    DeleteFramebuffer(g_depthPrePassFramebuffer);
    DeleteFramebuffer(g_geometryFramebuffer);
    DeleteFramebuffer(g_resolveGeometryFramebuffer);
    DeleteFramebuffer(g_fxaaFramebuffer);
}

auto CreateRendererFramebuffers(const glm::vec2& scaledFramebufferSize) -> void {

    PROFILER_ZONESCOPEDN("CreateRendererFramebuffers");

    g_depthPrePassFramebuffer = CreateFramebuffer({
        .Label = "Depth PrePass",
        .DepthStencilAttachment = TFramebufferDepthStencilAttachmentDescriptor{
            .Label = "Depth",
            .Format = TFormat::D24_UNORM_S8_UINT,
            .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
            .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
            .ClearDepthStencil = { 1.0f, 0 },
        }
    });

    g_geometryFramebuffer = CreateFramebuffer({
        .Label = "Geometry",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = "GeometryAlbedo",
                .Format = TFormat::R8G8B8A8_SRGB,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            },
            TFramebufferColorAttachmentDescriptor{
                .Label = "GeometryNormals",
                .Format = TFormat::R32G32B32A32_FLOAT,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            },
        },
        .DepthStencilAttachment = TFramebufferExistingDepthStencilAttachmentDescriptor{
            .ExistingDepthTexture = g_depthPrePassFramebuffer.DepthStencilAttachment.value().Texture,
        }
    });

    g_resolveGeometryFramebuffer = CreateFramebuffer({
        .Label = "ResolveGeometry",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = "ResolvedGeometry",
                .Format = TFormat::R8G8B8A8_SRGB,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::Clear,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            },
        },
    });

    g_fxaaFramebuffer = CreateFramebuffer({
        .Label = "PostFX-FXAA",
        .ColorAttachments = {
            TFramebufferColorAttachmentDescriptor{
                .Label = "PostFX-FXAA-Buffer",
                .Format = TFormat::R8G8B8A8_SRGB,
                .Extent = TExtent2D(scaledFramebufferSize.x, scaledFramebufferSize.y),
                .LoadOperation = TFramebufferAttachmentLoadOperation::DontCare,
                .ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f }
            }
        }
    });
}

auto main(
    [[maybe_unused]] int32_t argc,
    [[maybe_unused]] char* argv[]) -> int32_t {

    PROFILER_ZONESCOPEDN("main");
    auto previousTimeInSeconds = glfwGetTime();

    TWindowSettings windowSettings = {
        .ResolutionWidth = 1920,
        .ResolutionHeight = 1080,
        .ResolutionScale = 1.0f,
        .WindowStyle = TWindowStyle::Windowed,
        .IsDebug = true,
        .IsVSyncEnabled = true,
    };

    if (glfwInit() == GLFW_FALSE) {
        spdlog::error("Glfw: Unable to initialize");
        return 0;
    }

    /*
     * Setup Application
     */
    const auto isWindowWindowed = windowSettings.WindowStyle == TWindowStyle::Windowed;
    glfwWindowHint(GLFW_DECORATED, isWindowWindowed ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, isWindowWindowed ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if (windowSettings.IsDebug) {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    }

    const auto primaryMonitor = glfwGetPrimaryMonitor();
    if (primaryMonitor == nullptr) {
        spdlog::error("Glfw: Unable to get the primary monitor");
        return 0;
    }

    const auto primaryMonitorVideoMode = glfwGetVideoMode(primaryMonitor);
    const auto screenWidth = primaryMonitorVideoMode->width;
    const auto screenHeight = primaryMonitorVideoMode->height;

    const auto windowWidth = windowSettings.ResolutionWidth;
    const auto windowHeight = windowSettings.ResolutionHeight;

    auto monitor = windowSettings.WindowStyle == TWindowStyle::FullscreenExclusive
        ? primaryMonitor
        : nullptr;

    g_window = glfwCreateWindow(windowWidth, windowHeight, "How To Script", monitor, nullptr);
    if (g_window == nullptr) {
        spdlog::error("Glfw: Unable to create a window");
        return 0;
    }

    int32_t monitorLeft = 0;
    int32_t monitorTop = 0;
    glfwGetMonitorPos(primaryMonitor, &monitorLeft, &monitorTop);
    if (isWindowWindowed) {
        glfwSetWindowPos(g_window, screenWidth / 2 - windowWidth / 2 + monitorLeft, screenHeight / 2 - windowHeight / 2 + monitorTop);
    } else {
        glfwSetWindowPos(g_window, monitorLeft, monitorTop);
    }

    glfwSetKeyCallback(g_window, OnWindowKey);
    glfwSetCursorPosCallback(g_window, OnWindowCursorPosition);
    glfwSetCursorEnterCallback(g_window, OnWindowCursorEntered);    
    glfwSetFramebufferSizeCallback(g_window, OnWindowFramebufferSizeChanged);
    glfwSetInputMode(g_window, GLFW_CURSOR, g_cursorIsActive ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

    int32_t windowFramebufferWidth = 0;
    int32_t windowFramebufferHeight = 0;
    glfwGetFramebufferSize(g_window, &windowFramebufferWidth, &windowFramebufferHeight);
    OnWindowFramebufferSizeChanged(g_window, windowFramebufferWidth, windowFramebufferHeight);

    glfwMakeContextCurrent(g_window);
    if (!InitializeRhi()) {
        spdlog::error("Glad: Unable to initialize");
        return 0;
    }

    if (windowSettings.IsDebug) {
        glDebugMessageCallback(OnOpenGLDebugMessage, nullptr);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }

#ifdef USE_PROFILER
    TracyGpuContext
#endif

    g_imguiContext = ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_IsSRGB; // this little shit doesn't do anything
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    constexpr auto fontSize = 18.0f;

    io.Fonts->AddFontFromFileTTF("data/fonts/HurmitNerdFont-Regular.otf", fontSize);
    {
        constexpr float iconFontSize = fontSize * 4.0f / 5.0f;
        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.GlyphMinAdvanceX = iconFontSize;
        icons_config.GlyphOffset.y = 0; // Hack to realign the icons
        io.Fonts->AddFontFromFileTTF("data/fonts/" FONT_ICON_FILE_NAME_FAR, iconFontSize, &icons_config, icons_ranges);
    }
    {
        constexpr float iconFontSize = fontSize;
        static const ImWchar icons_ranges[] = {ICON_MIN_MD, ICON_MAX_16_MD, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.GlyphMinAdvanceX = iconFontSize;
        icons_config.GlyphOffset.y = 4; // Hack to realign the icons
        io.Fonts->AddFontFromFileTTF("data/fonts/" FONT_ICON_FILE_NAME_MD, iconFontSize, &icons_config, icons_ranges);
    }

    auto& imguiStyle = ImGui::GetStyle();
    imguiStyle.WindowMenuButtonPosition = ImGuiDir_None;
    imguiStyle.DisabledAlpha = 0.3f;
    imguiStyle.PopupRounding = 0;
    imguiStyle.WindowPadding = ImVec2(4, 4);
    imguiStyle.FramePadding = ImVec2(6, 4);
    imguiStyle.ItemSpacing = ImVec2(6, 2);
    imguiStyle.ScrollbarSize = 18;
    imguiStyle.WindowBorderSize = 1;
    imguiStyle.ChildBorderSize = 1;
    imguiStyle.PopupBorderSize = 1;
    imguiStyle.FrameBorderSize = 1;
    imguiStyle.WindowRounding = 0;
    imguiStyle.ChildRounding = 0;
    imguiStyle.FrameRounding = 0;
    imguiStyle.ScrollbarRounding = 0;
    imguiStyle.GrabRounding = 0;
    imguiStyle.AntiAliasedFill = true;
    imguiStyle.AntiAliasedLines = true;

    auto* colors = imguiStyle.Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
    colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.36f, 0.16f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.90f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.42f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.17f, 0.17f, 0.17f, 0.35f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.32f, 0.32f, 0.32f, 0.59f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.36f, 0.16f, 0.96f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.12f, 0.12f, 0.12f, 0.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.36f, 0.36f, 0.36f, 0.77f);
    colors[ImGuiCol_Separator] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.70f, 0.67f, 0.60f, 0.29f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.36f, 0.16f, 0.96f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.46f, 0.24f, 1.00f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.69f, 0.30f, 1.00f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.00f, 0.96f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);


    if (!ImGui_ImplGlfw_InitForOpenGL(g_window, true)) {
        spdlog::error("ImGui: Unable to initialize the GLFW backend");
        return 0;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 460")) {
        spdlog::error("ImGui: Unable to initialize the OpenGL backend");
        return 0;
    }

    glfwSwapInterval(windowSettings.IsVSyncEnabled ? 1 : 0);

    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.03f, 0.05f, 0.07f, 1.0f);

    std::vector<float> frameTimes;
    frameTimes.resize(512);
    std::fill_n(frameTimes.begin(), frameTimes.size(), 60.0f);

    auto isSrgbDisabled = false;
    auto isCullfaceDisabled = false;

    g_sceneViewerSize = g_windowFramebufferSize;
    auto scaledFramebufferSize = glm::vec2(g_sceneViewerSize) * windowSettings.ResolutionScale;

    /*
     * Renderer - Initialize Framebuffers
     */
    CreateRendererFramebuffers(scaledFramebufferSize);

    /*
     * Renderer - Initialize Pipelines
     */
    auto depthPrePassGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "Depth PrePass",
        .VertexShaderFilePath = "data/shaders/DepthPrePass.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/Depth.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .RasterizerState = {
            .FillMode = TFillMode::Solid,
            .CullMode = TCullMode::Back,
            .FaceWindingOrder = TFaceWindingOrder::CounterClockwise,
            .IsDepthBiasEnabled = false,
            .IsScissorEnabled = false,
            .IsRasterizerDisabled = false
        },
        .OutputMergerState = {
            .ColorMask = TColorMaskBits::None,
            .DepthState = {
                .IsDepthTestEnabled = true,
                .IsDepthWriteEnabled = true,
                .DepthFunction = TDepthFunction::Less
            },
        }
    });
    g_depthPrePassGraphicsPipeline = *depthPrePassGraphicsPipelineResult;

    auto geometryGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "GeometryPipeline",
        .VertexShaderFilePath = "data/shaders/SimpleDeferred.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/SimpleDeferred.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .RasterizerState = {
            .FillMode = TFillMode::Solid,
            .CullMode = TCullMode::Back,
            .FaceWindingOrder = TFaceWindingOrder::CounterClockwise,
        },
        .OutputMergerState = {
            .DepthState = {
                .IsDepthTestEnabled = true,
                .IsDepthWriteEnabled = false,
                .DepthFunction = TDepthFunction::Equal,
            }
        }
    });
    if (!geometryGraphicsPipelineResult) {
        spdlog::error(geometryGraphicsPipelineResult.error());
        return -1;
    }
    g_geometryGraphicsPipeline = *geometryGraphicsPipelineResult;

    auto resolveGeometryGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "ResolveGeometryPipeline",
        .VertexShaderFilePath = "data/shaders/FST.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/ResolveDeferred.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .OutputMergerState = {
            .DepthState = {
                .IsDepthTestEnabled = true,
            }
        }
    });
    if (!resolveGeometryGraphicsPipelineResult) {
        spdlog::error(resolveGeometryGraphicsPipelineResult.error());
        return -1;
    }
    g_resolveGeometryGraphicsPipeline = *resolveGeometryGraphicsPipelineResult;

    auto fstGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "FST",
        .VertexShaderFilePath = "data/shaders/FST.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/FST.GammaCorrected.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles
        },
    });

    if (!fstGraphicsPipelineResult) {
        spdlog::error(fstGraphicsPipelineResult.error());
        return 0;
    }
    g_fstGraphicsPipeline = *fstGraphicsPipelineResult;

    auto debugLinesGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "DebugLinesPipeline",
        .VertexShaderFilePath = "data/shaders/DebugLines.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/DebugLines.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Lines,
            .IsPrimitiveRestartEnabled = false,
        },
        .VertexInput = TVertexInputDescriptor{
            .VertexInputAttributes = {
                TVertexInputAttributeDescriptor{
                    .Location = 0,
                    .Binding = 0,
                    .Format = TFormat::R32G32B32_FLOAT,
                    .Offset = offsetof(TGpuDebugLine, StartPosition),
                },
                TVertexInputAttributeDescriptor{
                    .Location = 1,
                    .Binding = 0,
                    .Format = TFormat::R32G32B32A32_FLOAT,
                    .Offset = offsetof(TGpuDebugLine, StartColor),
                },
            }
        }
    });

    if (!debugLinesGraphicsPipelineResult) {
        spdlog::error(debugLinesGraphicsPipelineResult.error());
        return 0;
    }
    g_debugLinesGraphicsPipeline = *debugLinesGraphicsPipelineResult;

    auto fxaaGraphicsPipelineResult = CreateGraphicsPipeline({
        .Label = "FXAA",
        .VertexShaderFilePath = "data/shaders/FST.vs.glsl",
        .FragmentShaderFilePath = "data/shaders/FST.fs.glsl",
        .InputAssembly = {
            .PrimitiveTopology = TPrimitiveTopology::Triangles,
        },
        .RasterizerState = {
            .FillMode = TFillMode::Solid,
            .CullMode = TCullMode::Back,
            .FaceWindingOrder = TFaceWindingOrder::CounterClockwise,
        },
    });
    if (!fxaaGraphicsPipelineResult) {
        spdlog::error(fxaaGraphicsPipelineResult.error());
        return 0;
    }

    g_fxaaGraphicsPipeline = *fxaaGraphicsPipelineResult;

    g_debugLinesVertexBuffer = CreateBuffer("VertexBuffer-DebugLines", sizeof(TGpuDebugLine) * 16384, nullptr, GL_DYNAMIC_STORAGE_BIT);

    auto cameraPosition = glm::vec3{0.0f, 0.0f, 20.0f};
    auto cameraDirection = glm::vec3{0.0f, 0.0f, -1.0f};
    auto cameraUp = glm::vec3{0.0f, 1.0f, 0.0f};
    TGpuGlobalUniforms globalUniforms = {
        .ProjectionMatrix = glm::infinitePerspective(glm::radians(70.0f), scaledFramebufferSize.x / static_cast<float>(scaledFramebufferSize.y), 0.1f),
        .ViewMatrix = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, cameraUp),
        .CameraPosition = glm::vec4{cameraPosition, 0.0f}
    };
    auto globalUniformsBuffer = CreateBuffer("TGpuGlobalUniforms", sizeof(TGpuGlobalUniforms), &globalUniforms, GL_DYNAMIC_STORAGE_BIT);
    auto objectsBuffer = CreateBuffer("TGpuObjects", sizeof(TGpuObject) * 16384, nullptr, GL_DYNAMIC_STORAGE_BIT);

    g_gpuGlobalLights.push_back(TGpuGlobalLight{
        .ProjectionMatrix = glm::mat4(1.0f),
        .ViewMatrix = glm::mat4(1.0f),
        .Direction = glm::vec4(10.0f, 20.0f, 10.0f, 0.0f),
        .Strength = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)
    });

    auto globalLightsBuffer = CreateBuffer("SGpuGlobalLights", g_gpuGlobalLights.size() * sizeof(TGpuGlobalLight), g_gpuGlobalLights.data(), GL_DYNAMIC_STORAGE_BIT);

    g_fstSamplerNearestClampToEdge = GetOrCreateSampler({
       .AddressModeU = TTextureAddressMode::ClampToEdge,
       .AddressModeV = TTextureAddressMode::ClampToEdge,
       .MagFilter = TTextureMagFilter::Nearest,
       .MinFilter = TTextureMinFilter::NearestMipmapNearest
    });

    /*
     * Load Assets
     */

    //LoadModelFromFile("Test", "/home/deccer/Storage/Resources/Models/Sponza/glTF/Sponza.gltf");
    //LoadModelFromFile("Test", "/home/deccer/Storage/Resources/Models/_Random/SM_Cube_OneMaterialPerFace.gltf");
    //LoadModelFromFile("Test", "/home/deccer/Downloads/modular_ruins_c/modular_ruins_c.glb");
    LoadModelFromFile("Test", "data/default/SM_Deccer_Cubes_Textured.glb");
    //LoadModelFromFile("SM_Tower", "data/scenes/Tower/scene.gltf");
    
    //LoadModelFromFile("SM_DeccerCube", "data/scenes/stylized_low-poly_sand_block.glb");
    //LoadModelFromFile("SM_Cube1", "data/scenes/stylized_low-poly_sand_block.glb");
    //LoadModelFromFile("SM_Cube_1by1by1", "data/basic/SM_Cube_1by1by1.glb");
    
    //LoadModelFromFile("SM_Cube_1by1_x5", "data/basic/SM_Cube_1by1_x5.glb");
    //LoadModelFromFile("SM_Cube_1by1_y5", "data/basic/SM_Cube_1by1_y5.glb");
    //LoadModelFromFile("SM_Cube_1by1_z5", "data/basic/SM_Cube_1by1_z5.glb");
    //LoadModelFromFile("Test", "data/basic/Cubes/SM_Cube_1by1_5y.gltf");
    //LoadModelFromFile("Test2", "data/basic/Cubes2/SM_Cube_1by1_5y.gltf");
    
    //LoadModelFromFile("SM_IntelSponza", "data/scenes/IntelSponza/NewSponza_Main_glTF_002.gltf");

    uint32_t g_gridTexture;

    int32_t iconWidth = 0;
    int32_t iconHeight = 0;
    int32_t iconComponents = 0;

    auto gridTexturePixels = LoadImageFromFile("data/basic/T_Grid_1024_D.png", &iconWidth, &iconHeight, &iconComponents);
    if (gridTexturePixels != nullptr) {
        glCreateTextures(GL_TEXTURE_2D, 1, &g_gridTexture);
        auto mipLevels = static_cast<int32_t>(1 + glm::floor(glm::log2(glm::max(static_cast<float>(iconWidth), static_cast<float>(iconHeight)))));
        glTextureStorage2D(g_gridTexture, mipLevels, GL_SRGB8_ALPHA8, iconWidth, iconHeight);
        glTextureSubImage2D(g_gridTexture, 0, 0, 0, iconWidth, iconHeight, GL_RGBA, GL_UNSIGNED_BYTE, gridTexturePixels);
        glGenerateTextureMipmap(g_gridTexture);
        FreeImage(gridTexturePixels);
    }
    
    /// Setup Scene ////////////

    AddEntity(std::nullopt, "Test", glm::mat4(1.0f));

    //AddEntity(std::nullopt, "Test2", glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -10.0f}));
    //AddEntity(std::nullopt, "SM_Tower", glm::scale(glm::mat4(1.0f), {0.01f, 0.01f, 0.01f}) * glm::translate(glm::mat4(1.0f), {-5.0f, -2.0f, -10.0f}));
    //AddEntity(std::nullopt, "SM_Cube1", glm::translate(glm::mat4(1.0f), {0.0f, 10.0f, 10.0f}));
    //AddEntity(std::nullopt, "SM_IntelSponza", glm::translate(glm::mat4(1.0f), {0.0f, -10.0f, 0.0f}));

    /// Start Render Loop ///////

    uint64_t frameCounter = 0;
    auto accumulatedTimeInSeconds = 0.0;
    auto averageFramesPerSecond = 0.0f;

    auto updateInterval = 1.0f;
    while (!glfwWindowShouldClose(g_window)) {

        PROFILER_ZONESCOPEDN("Frame");
        auto currentTimeInSeconds = glfwGetTime();
        auto deltaTimeInSeconds = currentTimeInSeconds - previousTimeInSeconds;
        auto framesPerSecond = 1.0f / deltaTimeInSeconds;
        accumulatedTimeInSeconds += deltaTimeInSeconds;

        frameTimes[frameCounter % frameTimes.size()] = framesPerSecond;
        if (accumulatedTimeInSeconds >= updateInterval) {
            auto sum = 0.0f;
            for (auto i = 0; i < frameTimes.size(); i++) {
                sum += frameTimes[i];
            }
            averageFramesPerSecond = sum / frameTimes.size();
            accumulatedTimeInSeconds = 0.0f;
        }

        ///////////////////////
        // Create Gpu Resources if necessary
        ///////////////////////
        auto createGpuResourcesNecessaryView = g_gameRegistry.view<TComponentCreateGpuResourcesNecessary>();
        for (auto& entity : createGpuResourcesNecessaryView) {

            PROFILER_ZONESCOPEDN("Create Gpu Resources");
            auto& meshComponent = g_gameRegistry.get<TComponentMesh>(entity);
            auto& materialComponent = g_gameRegistry.get<TComponentMaterial>(entity);

            CreateGpuMesh(meshComponent.Mesh);
            CreateCpuMaterial(materialComponent.Material);

            g_gameRegistry.emplace<TComponentGpuMesh>(entity, meshComponent.Mesh);
            g_gameRegistry.emplace<TComponentGpuMaterial>(entity, materialComponent.Material);

            g_gameRegistry.remove<TComponentCreateGpuResourcesNecessary>(entity);
        }

        // Resize if necessary
        if (g_windowFramebufferResized || g_sceneViewerResized) {

            PROFILER_ZONESCOPEDN("Resized");

            g_windowFramebufferScaledSize = glm::ivec2{g_windowFramebufferSize.x * windowSettings.ResolutionScale, g_windowFramebufferSize.y * windowSettings.ResolutionScale};
            g_sceneViewerScaledSize = glm::ivec2{g_sceneViewerSize.x * windowSettings.ResolutionScale, g_sceneViewerSize.y * windowSettings.ResolutionScale};

            if (g_isEditor) {
                scaledFramebufferSize = g_sceneViewerScaledSize;
            } else {
                scaledFramebufferSize = g_windowFramebufferScaledSize;
            }

            DeleteRendererFramebuffers();
            CreateRendererFramebuffers(scaledFramebufferSize);

            g_windowFramebufferResized = false;
            g_sceneViewerResized = false;
        }

        // Update State
        HandleCamera(static_cast<float>(deltaTimeInSeconds));

        // Update Per Frame Uniforms

        globalUniforms.ProjectionMatrix = glm::infinitePerspective(glm::radians(60.0f), (float)scaledFramebufferSize.x / (float)scaledFramebufferSize.y, 0.1f);
        globalUniforms.ViewMatrix = g_mainCamera.GetViewMatrix();
        globalUniforms.CameraPosition = glm::vec4(g_mainCamera.Position, 0.0f);
        UpdateBuffer(globalUniformsBuffer, 0, sizeof(TGpuGlobalUniforms), &globalUniforms);

        //
        // CLEAR DEBUG LINES
        //
        if (g_drawDebugLines) {
            g_debugLines.clear();

            g_debugLines.push_back(TGpuDebugLine{
                .StartPosition = glm::vec3{-150, 30, 4},
                .StartColor = glm::vec4{0.3f, 0.95f, 0.1f, 1.0f},
                .EndPosition = glm::vec3{150, -40, -4},
                .EndColor = glm::vec4{0.6f, 0.1f, 0.0f, 1.0f}
            });
        }

        if (isSrgbDisabled) {
            glEnable(GL_FRAMEBUFFER_SRGB);
            isSrgbDisabled = false;
        }

        {
            PROFILER_ZONESCOPEDN("All Depth PrePass Geometry");
            PushDebugGroup("Depth PrePass");
            BindFramebuffer(g_depthPrePassFramebuffer);
            {
                g_depthPrePassGraphicsPipeline.Bind();
                g_depthPrePassGraphicsPipeline.BindBufferAsUniformBuffer(globalUniformsBuffer, 0);

                const auto& renderablesView = g_gameRegistry.view<TComponentGpuMesh, TComponentTransform>();
                renderablesView.each([](
                    const auto& meshComponent,
                    const auto& transformComponent) {

                    PROFILER_ZONESCOPEDN("Draw PrePass Geometry");

                    auto& gpuMesh = GetGpuMesh(meshComponent.GpuMesh);

                    g_depthPrePassGraphicsPipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexPositionBuffer, 1);
                    g_depthPrePassGraphicsPipeline.SetUniform(0, transformComponent * gpuMesh.InitialTransform);

                    g_depthPrePassGraphicsPipeline.DrawElements(gpuMesh.IndexBuffer, gpuMesh.IndexCount);
                });
            }
            PopDebugGroup();
        }
        {
            PROFILER_ZONESCOPEDN("Draw Geometry All");
            PushDebugGroup("Geometry");
            BindFramebuffer(g_geometryFramebuffer);
            {
                g_geometryGraphicsPipeline.Bind();
                g_geometryGraphicsPipeline.BindBufferAsUniformBuffer(globalUniformsBuffer, 0);

                const auto& renderablesView = g_gameRegistry.view<TComponentGpuMesh, TComponentGpuMaterial, TComponentTransform>();
                renderablesView.each([](
                    const auto& meshComponent,
                    const auto& materialComponent,
                    const auto& transformComponent) {

                    PROFILER_ZONESCOPEDN("Draw Geometry");

                    auto& cpuMaterial = GetCpuMaterial(materialComponent.GpuMaterial);
                    auto& gpuMesh = GetGpuMesh(meshComponent.GpuMesh);

                    g_geometryGraphicsPipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexPositionBuffer, 1);
                    g_geometryGraphicsPipeline.BindBufferAsShaderStorageBuffer(gpuMesh.VertexNormalUvTangentBuffer, 2);
                    g_geometryGraphicsPipeline.SetUniform(0, transformComponent * gpuMesh.InitialTransform);

                    g_geometryGraphicsPipeline.BindTextureAndSampler(8, cpuMaterial.BaseColorTextureId,
                                                                     cpuMaterial.BaseColorTextureSamplerId);
                    g_geometryGraphicsPipeline.BindTextureAndSampler(9, cpuMaterial.NormalTextureId, cpuMaterial.NormalTextureSamplerId);

                    g_geometryGraphicsPipeline.DrawElements(gpuMesh.IndexBuffer, gpuMesh.IndexCount);
                });
            }
            PopDebugGroup();
        }
        {
            PROFILER_ZONESCOPEDN("Draw ResolveGeometry");
            PushDebugGroup("ResolveGeometry");
            BindFramebuffer(g_resolveGeometryFramebuffer);
            {
                g_resolveGeometryGraphicsPipeline.Bind();
                //auto& samplerId = g_samplers[size_t(g_fullscreenSamplerNearestClampToEdge.Id)]
                g_resolveGeometryGraphicsPipeline.BindBufferAsUniformBuffer(globalLightsBuffer, 2);
                g_resolveGeometryGraphicsPipeline.BindTexture(0, g_geometryFramebuffer.ColorAttachments[0]->Texture.Id);
                g_resolveGeometryGraphicsPipeline.BindTexture(1, g_geometryFramebuffer.ColorAttachments[1]->Texture.Id);

                g_resolveGeometryGraphicsPipeline.DrawArrays(0, 3);
            }
            PopDebugGroup();
        }

        /////////////// Debug Lines // move out to some LineRenderer

        if (g_drawDebugLines && !g_debugLines.empty()) {

            PROFILER_ZONESCOPEDN("Draw DebugLines");

            PushDebugGroup("Debug Lines");
            glDisable(GL_CULL_FACE);

            UpdateBuffer(g_debugLinesVertexBuffer, 0, sizeof(TGpuDebugLine) * g_debugLines.size(), g_debugLines.data());

            g_debugLinesGraphicsPipeline.Bind();
            g_debugLinesGraphicsPipeline.BindBufferAsVertexBuffer(g_debugLinesVertexBuffer, 0, 0, sizeof(TGpuDebugLine) / 2);
            g_debugLinesGraphicsPipeline.BindBufferAsUniformBuffer(globalUniformsBuffer, 0);
            g_debugLinesGraphicsPipeline.DrawArrays(0, g_debugLines.size() * 2);

            glEnable(GL_CULL_FACE);
            PopDebugGroup();
        }

        if (g_isFxaaEnabled) {
            PROFILER_ZONESCOPEDN("PostFX FXAA");
            PushDebugGroup("PostFX FXAA");
            BindFramebuffer(g_fxaaFramebuffer);
            {
                g_fxaaGraphicsPipeline.Bind();
                g_fxaaGraphicsPipeline.BindTexture(0, g_resolveGeometryFramebuffer.ColorAttachments[0]->Texture.Id);
                g_fxaaGraphicsPipeline.SetUniform(1, 1);
                g_fxaaGraphicsPipeline.DrawArrays(0, 3);
            }
            PopDebugGroup();
        }

        /////////////// UI

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (g_isEditor) {
            glClear(GL_COLOR_BUFFER_BIT);
        } else {
            glBlitNamedFramebuffer(g_isFxaaEnabled
                ? g_fxaaFramebuffer.Id
                : g_resolveGeometryFramebuffer.Id,
                0,
                0, 0, scaledFramebufferSize.x, scaledFramebufferSize.y,
                0, 0, g_windowFramebufferSize.x, g_windowFramebufferSize.y,
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (g_isEditor) {
            ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
        }

        if (ImGui::SliderFloat("Resolution Scale", &windowSettings.ResolutionScale, 0.05f, 4.0f, "%.2f")) {

            g_sceneViewerResized = g_isEditor;
            g_windowFramebufferResized = !g_isEditor;
        }
        ImGui::Checkbox("Enable FXAA", &g_isFxaaEnabled);

        if (!g_isEditor) {
            ImGui::SetNextWindowPos({32, 32});
            ImGui::SetNextWindowSize({168, 192});
            auto windowBackgroundColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
            windowBackgroundColor.w = 0.4f;
            ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBackgroundColor);
            if (ImGui::Begin("#InGameStatistics", nullptr, ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoDecoration)) {

                ImGui::TextColored(ImVec4{0.9f, 0.7f, 0.0f, 1.0f}, "F1 to toggle editor");
                ImGui::SeparatorText("Frame Statistics");

                ImGui::Text("afps: %.0f rad/s", glm::two_pi<float>() * framesPerSecond);
                ImGui::Text("dfps: %.0f °/s", glm::degrees(glm::two_pi<float>() * framesPerSecond));
                ImGui::Text("mfps: %.0f", averageFramesPerSecond);
                ImGui::Text("rfps: %.0f", framesPerSecond);
                ImGui::Text("rpms: %.0f", framesPerSecond * 60.0f);
                ImGui::Text("  ft: %.2f ms", deltaTimeInSeconds * 1000.0f);
                ImGui::Text("   f: %lu", frameCounter);
            }
            ImGui::End();
            ImGui::PopStyleColor();
        }

        if (g_isEditor) {
            if (ImGui::BeginMainMenuBar()) {
                //ImGui::Image(reinterpret_cast<ImTextureID>(g_iconPackageGreen), ImVec2(16, 16), g_imvec2UnitY, g_imvec2UnitX);
                ImGui::SetCursorPosX(20.0f);
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Quit")) {

                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            // Scene Viewer
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            if (ImGui::Begin(ICON_FA_WORM "Scene")) {
                auto currentSceneWindowSize = ImGui::GetContentRegionAvail();
                if ((currentSceneWindowSize.x != g_sceneViewerSize.x || currentSceneWindowSize.y != g_sceneViewerSize.y)) {
                    g_sceneViewerSize = glm::ivec2(currentSceneWindowSize.x, currentSceneWindowSize.y);
                    g_sceneViewerResized = true;
                }

                auto texture = g_geometryFramebuffer.ColorAttachments[1].value().Texture.Id;
                auto imagePosition = ImGui::GetCursorPos();
                ImGui::Image(static_cast<intptr_t>(texture), currentSceneWindowSize, g_imvec2UnitY, g_imvec2UnitX);
                ImGui::SetCursorPos(imagePosition);
                if (ImGui::BeginChild(1, ImVec2{192, -1})) {
                    if (ImGui::CollapsingHeader("Statistics")) {
                    }
                }
                ImGui::EndChild();
            }
            ImGui::PopStyleVar();
            ImGui::End();
        }        
        {
            PROFILER_ZONESCOPEDN("Draw ImGUI");
            ImGui::Render();
            auto* imGuiDrawData = ImGui::GetDrawData();
            if (imGuiDrawData != nullptr) {
                PushDebugGroup("UI");
                glDisable(GL_FRAMEBUFFER_SRGB);
                isSrgbDisabled = true;
                glViewport(0, 0, g_windowFramebufferSize.x, g_windowFramebufferSize.y);
                ImGui_ImplOpenGL3_RenderDrawData(imGuiDrawData);
                PopDebugGroup();
            }
        }
        {
            PROFILER_ZONESCOPEDN("SwapBuffers");
            glfwSwapBuffers(g_window);
            frameCounter++;
        }

#ifdef USE_PROFILER
        TracyGpuCollect;
        FrameMark;
#endif

        glfwPollEvents();

        if (!g_windowHasFocus && g_sleepWhenWindowHasNoFocus) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        previousTimeInSeconds = currentTimeInSeconds;
    }

    /// Cleanup Resources //////

    DeleteBuffer(globalLightsBuffer);
    DeleteBuffer(globalUniformsBuffer);
    DeleteBuffer(objectsBuffer);

    DeleteRendererFramebuffers();

    DeletePipeline(g_debugLinesGraphicsPipeline);
    DeletePipeline(g_fstGraphicsPipeline);

    DeletePipeline(g_depthPrePassGraphicsPipeline);
    DeletePipeline(g_geometryGraphicsPipeline);
    DeletePipeline(g_resolveGeometryGraphicsPipeline);
    DeletePipeline(g_fxaaGraphicsPipeline);

    DeleteTextures();

    UnloadRhi();

    return 0;
}
