///--------------------------------------------------------------------------------
///-- Author        ReactiioN
///-- Copyright     2016-2020, ReactiioN
///-- License       MIT
///--------------------------------------------------------------------------------
#pragma once

#include <valve-bsp-parser/core/valve_structs.hpp>
#include <shared_mutex>
#include <LzmaLib.h>

namespace rn {
class bsp_parser final
{
public:
    bsp_parser() = default;

    ~bsp_parser() = default;

    bsp_parser(
        const bsp_parser& rhs
    ) = delete;

    bsp_parser& operator = (
        const bsp_parser& rhs
    ) = delete;

    bsp_parser(
        bsp_parser&& rhs
    ) noexcept;

    bsp_parser& operator = (
        bsp_parser&& rhs
    ) noexcept;

private:
    bool set_current_map(
        const std::string& directory,
        const std::string& map_name,
        std::string&       file_path
    );

    bool parse_planes(
        std::ifstream& file
    );

    bool parse_nodes(
        std::ifstream& file
    );

    bool parse_leaffaces(
        std::ifstream& file
    );

    bool parse_leafbrushes(
        std::ifstream& file
    ); 
    
    //bool parse_entities(
    //    std::ifstream& file
    //);

    bool parse_polygons();

    void ray_cast_node(
        std::int32_t    node_index,
        float           start_fraction,
        float           end_fraction,
        const vector3&  origin,
        const vector3&  destination,
        valve::trace_t* out
    );

    void ray_cast_surface(
        std::int32_t    surface_index,
        const vector3&  origin,
        const vector3&  destination,
        valve::trace_t* out
    );

    void ray_cast_brush(
        valve::dbrush_t* brush,
        const vector3&   origin,
        const vector3&   destination,
        valve::trace_t*  out
    )const;

    template<typename type>
    NODISCARD
    bool parse_lump(
        std::ifstream&          file,
        const valve::lump_index lump_index,
        std::vector<type>&      out
    ) const
    {
        const auto index = static_cast<std::underlying_type_t<valve::lump_index>>( lump_index );
        if( index >= _bsp_header.lumps.size() ) {
            return false;
        }
        //TODO: decompress lump here if compressed
        //Default behavior is casting data to underlying types.
        //It doesn't handle compression
        

        //There are two exceptions though: Game lumps (35) (yes, multiple; compressed individually), and PAK Lump (40) (basically a zip file)


        const auto& lump = _bsp_header.lumps.at( index );
        const auto size  = static_cast<std::size_t>( lump.file_size ) / sizeof( type );

        //out.resize( size );

        
        file.seekg( lump.file_offset );
        //Temporarily make this array. This data may be compressed.
        char* tmpData = new char[lump.file_size + 1];
        //bool isCompressed = false;
        memset(tmpData, 0, lump.file_size + 1);

        file.read(tmpData, lump.file_size);

        lzma_header_t lzma_header;

        memcpy(&lzma_header, tmpData, sizeof(lzma_header));

        if (has_valid_lzma_ident(lzma_header.id))
        {
            assert(lump_index != valve::lump_index::game_lump || lump_index != valve::lump_index::pak_file); //Those have special rules regarding compression.
            out.resize(static_cast<std::size_t>(lzma_header.actualSize) / sizeof(type));
            LzmaUncompress(reinterpret_cast<char*>(out.data()), lzma_header.actualSize, tmpData, lzma_header.lzmaSize, &lzma_header.properties, LZMA_PROPS_SIZE);
        }
        else
        {
            out.resize(size);
            out.assign(tmpData,tmpData+lump.file_size);
        }

        


        //file.read( reinterpret_cast<char*>( out.data() ), size * static_cast<std::size_t>( sizeof( type ) ) );
        delete[] tmpData;
        return true;
    }

public:
    bool load_map(
        const std::string& directory,
        const std::string& map_name
    );

    bool is_visible(
        const vector3& origin,
        const vector3& destination
    );

    void trace_ray(
        const vector3&  origin,
        const vector3&  final,
        valve::trace_t* out
    );




    //TODO: Should all of this be public?
private:
    std::string                      _map_name;
    valve::dheader_t                 _bsp_header;
    //entities go here
    std::vector<valve::mvertex_t>    _vertices;
    std::vector<valve::cplane_t>     _planes;
    std::vector<valve::dedge_t>      _edges;
    std::vector<std::int32_t>        _surf_edges;
    std::vector<valve::dleaf_t>      _leaves;
    std::vector<valve::snode_t>      _nodes;
    std::vector<valve::dface_t>      _surfaces;
    std::vector<valve::texinfo_t>    _tex_infos;
    std::vector<valve::dbrush_t>     _brushes;
    std::vector<valve::dbrushside_t> _brush_sides;
    std::vector<std::uint16_t>       _leaf_faces;
    std::vector<std::uint16_t>       _leaf_brushes;
    std::vector<valve::polygon>      _polygons;
    mutable std::shared_timed_mutex  _mutex;
};
}
