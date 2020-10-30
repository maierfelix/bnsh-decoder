meta:
  id: bnsh
  file-extension: bnsh
  endian: le
seq:
  - id: header
    type: header
instances:
  blocks:
    pos: header.ofs_first_block
    type: block_header
  rlt:
    pos: header.ofs_relocation_table
    type: relocation_table
types:
  header:
    seq:
      - id: magic
        size: 8
      - id: version
        size: 4
      - id: byte_order_mark
        type: u2
      - id: alignment_shift
        type: u1
      - id: target_address_size
        type: u1
      - id: ofs_file_name
        type: u4
      - id: flag
        type: u2
      - id: ofs_first_block
        type: u2
      - id: ofs_relocation_table
        type: u4
      - id: file_size
        type: u4
    instances:
      shader_name:
        pos: ofs_file_name
        type: str
        terminator: 0
        encoding: UTF-8
  block_header:
    seq:
      - id: magic
        type: str
        encoding: ASCII
        size: 4
      - id: ofs_next_block
        type: u4
      - id: block_size
        type: u4
      - id: reserved
        type: u4
      - id: block
        type:
          switch-on: magic
          cases:
            '"grsc"': grsc_block
            "'_STR'": string_block
    instances:
      next_block:
        pos: ofs_next_block + 0x60
        type: block_header
  grsc_block:
    seq:
      - id: target_api_type
        type: u2
      - id: target_api_version
        type: u2
      - id: target_code_type
        type: u2
      - id: reserved_0
        size: 0x2
      - id: compiler_version
        size: 4
      - id: shader_variation_count
        type: u4
      - id: ofs_shader_variation_array
        type: u8
      - id: ofs_shader_binary_pool
        type: u8
      - id: llc_version
        size: 8
      - id: reserved_1
        size: 0x28
    instances:
      shader_variation_array:
        pos: ofs_shader_variation_array
        type: shader_variation_data
        repeat: expr
        repeat-expr: shader_variation_count
      shader_binary_pool:
        pos: ofs_shader_binary_pool
        type: shader_binary_pool
    types:
      shader_variation_data:
        seq:
          - id: ofs_source_program
            type: u8
          - id: ofs_intermediate_program
            type: u8
          - id: ofs_binary_program
            type: u8
          - id: ofs_parent
            type: u8
          - id: reserved
            size: 0x20
        instances:
          source_program:
            pos: ofs_source_program
            type: shader_program_data
            if: ofs_source_program != 0x0
          intermediate_program:
            pos: ofs_intermediate_program
            type: shader_program_data
            if: ofs_intermediate_program != 0x0
          binary_program:
            pos: ofs_binary_program
            type: shader_program_data
            if: ofs_binary_program != 0x0
      shader_program_data:
        seq:
          - id: info
            type: shader_info_data
          - id: object_size
            type: u4
          - id: reserved_0
            size: 0x4
          - id: ofs_object
            type: u8
          - id: ofs_parent
            type: u8
          - id: ofs_shader_reflection
            type: u8
          - id: reserved_1
            size: 0x20
          - id: reserved_2
            size: 0x8
          - id: ofs_shader_ir
            type: u8
        instances:
          shader_reflection_data:
            pos: ofs_shader_reflection
            type: shader_reflection_data
          parent:
            pos: ofs_parent
            type: shader_variation_data
      shader_reflection_data:
        seq:
          - id: ofs_vertex_reflection
            type: u8
          - id: ofs_hull_reflection
            type: u8
          - id: ofs_domain_reflection
            type: u8
          - id: ofs_geometry_reflection
            type: u8
          - id: ofs_fragment_reflection
            type: u8
          - id: ofs_compute_reflection
            type: u8
          - id: reserved_0
            size: 0x10
        instances:
          vertex_reflection:
            pos: ofs_vertex_reflection
            type: shader_reflection_stage_data
            if: ofs_vertex_reflection != 0
          hull_reflection:
            pos: ofs_hull_reflection
            type: shader_reflection_stage_data
            if: ofs_hull_reflection != 0
          domain_reflection:
            pos: ofs_domain_reflection
            type: shader_reflection_stage_data
            if: ofs_domain_reflection != 0
          geometry_reflection:
            pos: ofs_geometry_reflection
            type: shader_reflection_stage_data
            if: ofs_geometry_reflection != 0
          fragment_reflection:
            pos: ofs_fragment_reflection
            type: shader_reflection_stage_data
            if: ofs_fragment_reflection != 0
          compute_reflection:
            pos: ofs_compute_reflection
            type: shader_reflection_stage_data
            if: ofs_compute_reflection != 0
      shader_reflection_stage_data:
        seq:
          - id: shader_input_dictionary
            type: dictionary_entry
          - id: shader_output_dictionary
            type: dictionary_entry
          - id: sampler_dictionary
            type: dictionary_entry
          - id: constant_buffer_dictionary
            type: dictionary_entry
          - id: unordered_access_buffer_dictionary
            type: dictionary_entry
          - id: index_shader_output
            type: u4
          - id: index_sampler
            type: u4
          - id: index_constant_buffer
            type: u4
          - id: index_unordered_access_buffer
            type: u4
          - id: ofs_shader_slot_array
            type: u4
          - id: compute_workgroup_size_x
            type: u4
          - id: compute_workgroup_size_y
            type: u4
          - id: compute_workgroup_size_z
            type: u4
          - id: index_image
            type: u4
          - id: ofs_image_dictionary
            type: u4
          - id: reserved
            size: 0x8
        instances:
          shader_slot_array:
            pos: ofs_shader_slot_array
            type: u4
            repeat: expr
            repeat-expr: index_unordered_access_buffer
      dictionary_entry:
        seq:
          - id: ofs_entry
            type: u8
        instances:
          data:
            pos: ofs_entry
            type: dictionary_data
            if: ofs_entry != 0x0
      dictionary_data:
        seq:
          - id: magic
            contents: '_DIC'
          - id: str_count
            type: u4
          - id: padding
            type: u4
          - id: unk0
            type: u4
        instances:
          strings:
            pos: _parent.ofs_entry + 0x20
            type: str_entry
            repeat: expr
            repeat-expr: str_count
      shader_info_data:
        seq:
          - id: flags
            type: u2
          - id: code_type
            type: u1
          - id: source_format
            type: u1
          - id: reserved_0
            size: 0x4
          - id: ofs_vertex_shader_code
            type: u4
          - id: binary_format
            type: u8
          - id: ofs_hull_shader_code
            type: u8
          - id: ofs_domain_shader_code
            type: u8
          - id: ofs_geometry_shader_code
            type: u4
          - id: ofs_fragment_shader_code
            type: u8
          - id: ofs_compute_shader_code
            type: u8
          - id: reserved_1
            size: 0x28
        instances:
          vertex_source_array_code:
            pos: ofs_vertex_shader_code
            type: source_array_code
            if: ofs_vertex_shader_code != 0
          hull_source_array_code:
            pos: ofs_hull_shader_code
            type: source_array_code
            if: ofs_hull_shader_code != 0
          domain_shader_code:
            pos: ofs_domain_shader_code
            type: source_array_code
            if: ofs_domain_shader_code != 0
          geometry_shader_code:
            pos: ofs_geometry_shader_code
            type: source_array_code
            if: ofs_geometry_shader_code != 0
          fragment_shader_code:
            pos: ofs_fragment_shader_code
            type: source_array_code
            if: ofs_fragment_shader_code != 0
          compute_shader_code:
            pos: ofs_compute_shader_code
            type: source_array_code
            if: ofs_compute_shader_code != 0
      source_array_code:
        seq:
          - id: code_array_length
            type: u2
          - id: reserved_0
            size: 0x6
          - id: ofs_code_size_array
            type: u8
          - id: ofs_code_ptr_array
            type: u8
          - id: reserved_1
            size: 0x8
      shader_binary_pool:
        seq:
          - id: memory_pool_property
            type: u4
          - id: memory_size
            type: u4
          - id: ofs_memory
            type: u8
          - id: reserved
            size: 0x10
        instances:
          byte_code:
            pos: ofs_memory
            type: bytecode_entry
            size: memory_size
          const_data:
            pos: 0x910
            type: shader_const_data
      shader_const_data:
        seq:
          - id: ofs_unknown_0
            type: u8
          - id: len_bytecode
            type: u4
          - id: len_const_data
            type: u4
          - id: ofs_const_block_start
            type: u4
          - id: ofs_const_block_end
            type: u4
          - id: maybe_bytecode_header_padding
            contents: [0x30, 0x0, 0x0, 0x0]
          - id: unk_0
            type: u4
        instances:
          data:
            pos: _parent.ofs_memory + ofs_const_block_start
            if: len_const_data > 0
            type: f4
            repeat: expr
            repeat-expr: len_const_data / 0x4
      bytecode_entry:
        seq:
          - id: magic
            size: 0x4
            contents: [0x78, 0x56, 0x34, 0x12]
          - id: reserved
            size: 0x2C
          - id: header
            type: bytecode_header
            size: 0x50
          - id: body
            size-eos: true
      bytecode_header:
        seq:
          - id: common_word_0
            type: common_word_0
          - id: common_word_1
            type: u4
          - id: common_word_2
            type: u4
          - id: common_word_3
            type: u4
          - id: common_word_4
            type: u4
          - id: io
            #Need to properly parse sph_type but kaitai
            size-eos: true
            type:
              switch-on: 2
              cases:
                1: io_1
                2: io_2
        types:
          #Kaitai is doing some weird bitfield shit here
          #So these will remain unused for now
          common_word_0:
            seq:
              - id: sph_type
                type: b5
              - id: version
                type: b5
              - id: shader_type
                type: b4
              - id: mrt_enable
                type: b1
              - id: kills_pixels
                type: b1
              - id: does_global_store
                type: b1
              - id: sass_version
                type: b4
              - id: reserved
                type: b5
              - id: does_load_or_store
                type: b1
              - id: does_fp_64
                type: b1
              - id: stream_out_mask
                type: b4
          common_word_1:
            seq:
              - id: memory_low_size
                type: b24
              - id: attribute_count
                type: b8
          common_word_2:
            seq:
              - id: memory_high_size
                type: b24
              - id: threads_count
                type: b8
          common_word_3:
            seq:
              - id: memory_crs_size
                type: b24
              - id: output_topology
                type: b4
              - id: reserved
                type: b4
          common_word_4:
            seq:
              - id: max_vertex_count
                type: b12
              - id: store_req_start
                type: b8
              - id: reserved
                type: b4
              - id: store_req_end
                type: b4
          io_1:
            seq:
              - id: i_sys_val_a
                type: b24
              - id: i_sys_val_b
                type: b8
              - id: i_vec
                type: b4
                repeat: expr
                repeat-expr: 32
              - id: i_color
                type: u2
              - id: i_sys_val_c
                type: u2
              - id: i_fixed_texture
                type: b4
                repeat: expr
                repeat-expr: 10
              - id: i_reserved
                type: u1
              - id: o_sys_vals
                type: u4
              - id: o_vec
                type: b4
                repeat: expr
                repeat-expr: 32
              - id: o_col
                type: u2
              - id: o_sys_val_c
                type: u2
              - id: o_fixed_texture
                type: b4
                repeat: expr
                repeat-expr: 10
              - id: o_reserved
                type: u1
          io_2:
            seq:
              - id: i_sys_val_a
                type: b24
              - id: i_sys_val_b
                type: b8
              - id: i_vec
                type: u1
                repeat: expr
                repeat-expr: 32
              - id: i_col
                type: u2
              - id: i_sys_val_c
                type: u2
              - id: i_fixed_texture
                type: u1
                repeat: expr
                repeat-expr: 10
              - id: i_reserved
                type: u2
              - id: o_target
                type: b4
                repeat: expr
                repeat-expr: 8
              - id: o_mask
                type: b1
              - id: o_depth
                type: b1
              - id: o_reserved
                type: b30
  string_block:
    seq:
      - id: count
        type: u8
      - id: strings
        type: string_entry
        repeat: expr
        repeat-expr: count
    types:
      string_entry:
        seq:
          - id: len_string
            type: u2
          - id: string
            type: str
            size: len_string
            encoding: UTF-8
          - id: alignment
            size: 1 + (_io.pos + 1) % 2

  attribute_section:
    seq:
      - id: ofs_frag_indices
        type: u4
      - id: ofs_start_frag_indics
        type: u4
      - id: ofs_end_frag_indics
        type: u4
      - id: len_indices
        type: u4
      - id: ofs_indices
        type: u8
    instances:
      indices:
        pos: ofs_indices
        type: u4
        repeat: expr
        repeat-expr: len_indices - 1

  str_entry:
    seq:
      - id: ofs_str
        type: u4
      - id: padding
        type: u4
      - id: unk1
        type: u4
      - id: unk2
        type: u4
    instances:
      string:
        pos: ofs_str
        type: string
    types:
      string:
        seq:
          - id: str_len
            type: u2
          - id: str
            type: str
            size: str_len
            encoding: UTF-8

  section_control_table:
    seq:
      - id: magic
        contents: [0x34, 0x12, 0x76, 0x98]
      - id: count
        type: u4
      - id: unk0
        type: u4
      - id: unk1
        type: u4
      - id: unk2
        type: u4
      - id: ofs_unk0
        type: u4
      - id: ofs_unk1
        type: u4
      - id: unk5
        type: u4
      - id: ofs_unk2
        type: u4
      - id: unk7
        type: u4
      - id: unk8
        type: u4
      - id: unk9
        type: u4
      - id: unk10
        type: u4
      - id: unk11
        type: u4

  relocation_table:
    seq:
      - id: magic
        contents: '_RLT'
      - id: pos_rlt
        type: u4
      - id: count_section
        type: u8
      - id: sections
        type: relocation_section
        size: 0x18
        repeat: expr
        repeat-expr: count_section

    types:
      relocation_section:
        seq:
          - id: ptr
            type: u8
          - id: ptr_in_file
            type: u4
          - id: size
            type: u4
      relocation_entry:
        seq:
          - id: position
            type: u4
          - id: count_structure
            type: u2
          - id: count_offset
            type: u1
          - id: count_padding
            type: u1
