#pragma once

//! ������ ��� �������
enum plugin_string {

// *********** Image Viewer *********** // 
	ps_title,

	ps_read_in_progress,

	ps_cfg_sc_title,
	ps_cfg_sc_opt100,
	ps_cfg_sc_opt125,
	ps_cfg_sc_opt150,
	ps_cfg_sc_fit,
	ps_cfg_100,
	ps_cfg_interpolate,
	ps_cfg_i_lq,
	ps_cfg_i_hq,
	ps_cfg_i_bil,
	ps_cfg_i_bic,
	ps_cfg_i_nn,
	ps_cfg_i_hqbil,
	ps_cfg_i_hqbic,
	ps_cfg_bckgr,
	ps_cfg_bckgr_disabled,
	ps_cfg_bckgr_grid10,
	ps_cfg_bckgr_grid20,
	ps_cfg_bckgr_grid30,
	ps_cfg_exif_orient,
	ps_cfg_ra_tilte,
	ps_cfg_ra_0,
	ps_cfg_ra_1,
	ps_cfg_ra_3,
	ps_cfg_ra_5,
	ps_cfg_ra_10,
	ps_cfg_ask_delete,
	ps_cfg_use_analyze,
	ps_cfg_use_view,
	ps_cfg_use_quickview,
	ps_cfg_add_to_pm,
	ps_cfg_prefix,

	ps_del_title,
	ps_del_recycle,
	ps_del_delete,

	ps_ok,
	ps_cancel,

	ps_err_open,
	ps_err_del,
	ps_err_mrb,

	ps_op_open_first_image,
	ps_op_open_last_image,
	ps_op_open_next_image,
	ps_op_opensel_next_image,
	ps_op_open_prev_image,
	ps_op_delete_image,
	ps_op_show_next_page,
	ps_op_show_prev_page,
	ps_op_scale_optimal,
	ps_op_scale_fit,
	ps_op_scale_increase,
	ps_op_scale_decrease,
	ps_op_scale_set10,
	ps_op_scale_set20,
	ps_op_scale_set30,
	ps_op_scale_set40,
	ps_op_scale_set50,
	ps_op_scale_set60,
	ps_op_scale_set70,
	ps_op_scale_set80,
	ps_op_scale_set90,
	ps_op_scale_set100,
	ps_op_move_step_left,
	ps_op_move_step_right,
	ps_op_move_step_top,
	ps_op_move_step_bottom,
	ps_op_move_screen_left,
	ps_op_move_screen_right,
	ps_op_move_screen_top,
	ps_op_move_screen_bottom,
	ps_op_move_image_left,
	ps_op_move_image_right,
	ps_op_move_image_top,
	ps_op_move_image_bottom,
	ps_op_fullscreen_mode,
	ps_op_rotate_right,
	ps_op_rotate_left,
	ps_op_flip_hor,
	ps_op_flip_vert,
	ps_op_tile_mode,
	ps_op_close,
//   ***********************************   // 

// *********** Portrait Matrix *********** // 
	MTypeImage,
	MTypeBMP,
	MTypePNG,
	MTypeJPEG,
	MTypeTIFF,

	MFileExt,
	MTXT,
	MDAT,

	MMatrixSizeLabel,
	MMatrixSize,
	MIG,
	MKUSLAU,
	MSizeImage,

	MGrade,
	MShowImage,

	MBegin,
	MCancel,

	MError,
	MIncorrectData,
	MOk,

	MErrorFileDI,
	MErrorFileGG,
	MErrorFileIG,
	MErrorFileJG,
	MErrorFileKUSLAU,

	MErrorData
//   ***********************************   //
};
