#ifndef LITERAL_STRINGS_H
#define LITERAL_STRINGS_H

#define STR_DELCARE_STMT(var_name, lit_str) \
    extern const char* var_name;

#define LIT_STRINGS_LIST \
STR_DELCARE_STMT(g_str_or, "或") \
STR_DELCARE_STMT(g_str_and, "与") \
STR_DELCARE_STMT(g_str_connected, "连接") \
STR_DELCARE_STMT(g_str_disconnected, "断开") \
STR_DELCARE_STMT(g_str_unkonw_st, "未知状态") \
STR_DELCARE_STMT(g_str_create_file, "创建文件") \
STR_DELCARE_STMT(g_str_fail, "失败") \
STR_DELCARE_STMT(g_str_create_folder, "创建文件夹") \
STR_DELCARE_STMT(g_str_unit_ms, "ms") \
STR_DELCARE_STMT(g_str_unit_s, "s") \
STR_DELCARE_STMT(g_str_abnormal, "异常") \
STR_DELCARE_STMT(g_str_normal, "正常") \
STR_DELCARE_STMT(g_str_pwr_st, "电源状态") \
STR_DELCARE_STMT(g_str_detector_st, "探测器状态") \
STR_DELCARE_STMT(g_str_storage_st, "数据存储状态") \
STR_DELCARE_STMT(g_str_test_finished, "测试结束") \
STR_DELCARE_STMT(g_str_unknown_reason, "未知原因") \
STR_DELCARE_STMT(g_str_unknown_cmd, "未知命令") \
STR_DELCARE_STMT(g_str_plz_conn_dev_firstly, "请先连接设备") \
STR_DELCARE_STMT(g_str_cannt ,"不能") \
STR_DELCARE_STMT(g_str_exceed ,"超过") \
STR_DELCARE_STMT(g_str_scan_dura_time ,"扫描时间")\
STR_DELCARE_STMT(g_str_param_in_cfg_file_error ,"配置文件参数错误")\
STR_DELCARE_STMT(g_str_syssettings_error ,"设置参数错误")\
STR_DELCARE_STMT(g_str_should_be_in_range, "应在如下范围内")\
STR_DELCARE_STMT(g_str_should_be_one_val_of, "应为如下值之一")\
STR_DELCARE_STMT(g_str_is_should_not_be_empty, "不应为空")\
STR_DELCARE_STMT(g_str_plz_check ,"请检查！") \
STR_DELCARE_STMT(g_str_set_succeeds,"设置成功") \
STR_DELCARE_STMT(g_str_error, "错误") \
STR_DELCARE_STMT(g_str_param_in_cfg_file, "配置文件参数") \
STR_DELCARE_STMT(gs_str_actual_val, "实际值") \
STR_DELCARE_STMT(g_str_Byte, "Byte") \
STR_DELCARE_STMT(g_str_KB, "KB") \
STR_DELCARE_STMT(g_str_MB, "MB") \
STR_DELCARE_STMT(g_str_GB, "GB") \
STR_DELCARE_STMT(g_str_TB, "TB") \
STR_DELCARE_STMT(g_str_storage_space, "存储空间") \
STR_DELCARE_STMT(g_str_checking, "正在检测") \
STR_DELCARE_STMT(g_str_left_en, "left") \
STR_DELCARE_STMT(g_str_right_en, "right") \
STR_DELCARE_STMT(g_str_pressed_en, "pressed") \
STR_DELCARE_STMT(g_str_released_en, "released") \
STR_DELCARE_STMT(g_str_on_en, "on") \
STR_DELCARE_STMT(g_str_off_en, "off") \
STR_DELCARE_STMT(g_str_unknown_en, "unknown") \
STR_DELCARE_STMT(g_str_tcp_server_local_ip, "本地IP") \
STR_DELCARE_STMT(g_str_tcp_server_local_port, "本地端口") \
STR_DELCARE_STMT(g_str_tcp_server_startup, "启动") \
STR_DELCARE_STMT(g_str_tcp_server_shutdown, "关闭") \
STR_DELCARE_STMT(g_str_sptrum_range, "能谱范围") \
STR_DELCARE_STMT(g_str_pixel, "pixel")

LIT_STRINGS_LIST

#endif // LITERAL_STRINGS_H
