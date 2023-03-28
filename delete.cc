
 util::TimerTaskPtr delete_task(new util::TimerTask("deleter"));
    delete_task->onRun([this](util::TimerTaskPtr &t) {
        time_t now = std::time(nullptr);
        MLOG(Icafe, INFO) << "Delete task start on time: "
                          << std::put_time(std::localtime(&now), "%F %T");
        _timer_scheduler.schedule(t, nextDeleteDelaySecond() * 1000);
        // TODO: delete files created n days before.
        _handler.post([this]() {
            deleteExpiredFiles(AsdtContext::Instance()->getAsdtConfig().data_expired_days);
        });
    });
    
    
 void deleteExpiredFiles(int n_days);
 
 
void ICafeServer::deleteExpiredFiles(int n_days) {
    namespace fs = boost::filesystem;
    try {
        time_t ts_now          = std::time(NULL);
        time_t ts_before_ndays = ts_now - n_days * 24 * 60 * 60;
        auto results = ICafeCardDB::Instance()->query_by_state(ICafeCardState::CARD_FINISH,
            ICafeCardState::STATE_MAX, ts_before_ndays * 1000);
        // 1. Delete expired bag files;
        for (auto &entity : *results) {
            if (fs::exists(entity.origin_bag_path) && fs::is_directory(entity.origin_bag_path)) {
                MLOG(Icafe, INFO) << "Find entity path: " << entity.origin_bag_path;
                fs::remove_all(entity.origin_bag_path);
            }
        }
        // 2. Delete log
        struct tm tm_now         = *std::localtime(&ts_now);
        std::string download_dir = AsdtContext::Instance()->getAsdtConfig().download_dir;
        if (!fs::exists(download_dir)) {
            MLOG(Icafe, INFO) << "Download dir not exists: " << download_dir;
            return;
        }
        for (auto &carid : fs::directory_iterator(download_dir)) {
            MLOG(Icafe, INFO) << "Carid name: " << carid.path().filename().string();
            if (!fs::is_directory(carid)) {
                continue;
            }
            for (auto &date_dir : fs::directory_iterator(carid.path())) {
                std::string date_dir_name = date_dir.path().filename().string();
                if (!fs::is_directory(date_dir)) {
                    continue;
                }

                std::regex pattern("[0-9]{8}$");
                std::smatch regex_match;
                if (std::regex_match(date_dir_name, regex_match, pattern)) {
                    tm tm_result;
                    time_t dir_ts;
                    strptime(date_dir_name.c_str(), "%Y%m%d", &tm_result); // 将字符串转换为tm时间
                    dir_ts = mktime(&tm_result); // 将tm时间转换为秒时间
                    if (ts_now - dir_ts > n_days * 24 * 60 * 60) {
                        MLOG(Icafe, INFO) << "Find matched date: " << date_dir_name;
                        for (auto &log_it : fs::directory_iterator(date_dir.path())) {
                            if (!fs::is_directory(log_it)) {
                                continue;
                            }
                            std::regex pattern_log("[a-zA-Z0-9]+_[0-9]{14}_[a-zA-Z]+_LOG$");
                            std::smatch regex_match;
                            if (std::regex_match(log_it.path().filename().string(), regex_match,
                                    pattern_log)) {
                                MLOG(Icafe, INFO)
                                    << "Delete log: " << log_it.path().filename().string();

                                fs::remove_all(log_it);
                            }
                        }
                    }
                }
            }
        }
        // 3.Delete adb logs.
        if (!fs::exists("./logs")) {
            MLOG(Bag, INFO) << "Adb logs dir not exists !";
            return;
        }
        for (auto &log : fs::directory_iterator("./logs")) {
            if (!fs::exists(log)) {
                continue;
            }
            if (ts_now - fs::last_write_time(log) >
                AsdtContext::Instance()->getAsdtConfig().adb_log_expired_days * 24 * 60 * 60) {
                MLOG(Bag, INFO) << "Delete adb logs name: " << log.path().filename().string();

                MLOG(Bag, INFO) << "Adb logs last update time: "
                                << util::to_timestr(fs::last_write_time(log));
                fs::remove_all(log);
            }
        }
        // 4.Delete expired cutbag
        if ("asd-cloudbag" != util::get_host_name()) {
            if (!fs::exists(AsdtContext::Instance()->getAsdtConfig().cut_bag_dir)) {
                MLOG(Bag, ERROR) << "Cut bag dir not exists !";
                return;
            }
            for (auto &bag_dir :
                fs::directory_iterator(AsdtContext::Instance()->getAsdtConfig().cut_bag_dir)) {
                if (!fs::is_directory(bag_dir)) {
                    continue;
                }
                for (auto &bag_data_dir : fs::directory_iterator(bag_dir)) {
                    MLOG(Bag, INFO) << "bag_data_dir = " << bag_data_dir.path().filename().string();
                    if (!fs::exists(bag_data_dir)) {
                        continue;
                    }
                    if (fs::last_write_time(bag_data_dir) < ts_before_ndays) {
                        MLOG(Bag, INFO)
                            << "Delete cut bag name: " << bag_data_dir.path().filename().string()
                            << "its last write time: "
                            << util::to_timestr(fs::last_write_time(bag_data_dir));
                        fs::remove_all(bag_data_dir);
                    }
                }
            }
        }
    } catch (fs::filesystem_error &e) {
        MLOG(Bag, INFO) << "Delete expired files error: " << e.what();
    } catch (std::exception &other_e) {
        MLOG(Bag, INFO) << "Delete expired files other error: " << other_e.what();
    }
}void ICafeServer::deleteExpiredFiles(int n_days) {
    namespace fs = boost::filesystem;
    try {
        time_t ts_now          = std::time(NULL);
        time_t ts_before_ndays = ts_now - n_days * 24 * 60 * 60;
        auto results = ICafeCardDB::Instance()->query_by_state(ICafeCardState::CARD_FINISH,
            ICafeCardState::STATE_MAX, ts_before_ndays * 1000);
        // 1. Delete expired bag files;
        for (auto &entity : *results) {
            if (fs::exists(entity.origin_bag_path) && fs::is_directory(entity.origin_bag_path)) {
                MLOG(Icafe, INFO) << "Find entity path: " << entity.origin_bag_path;
                fs::remove_all(entity.origin_bag_path);
            }
        }
        // 2. Delete log
        struct tm tm_now         = *std::localtime(&ts_now);
        std::string download_dir = AsdtContext::Instance()->getAsdtConfig().download_dir;
        if (!fs::exists(download_dir)) {
            MLOG(Icafe, INFO) << "Download dir not exists: " << download_dir;
            return;
        }
        for (auto &carid : fs::directory_iterator(download_dir)) {
            MLOG(Icafe, INFO) << "Carid name: " << carid.path().filename().string();
            if (!fs::is_directory(carid)) {
                continue;
            }
            for (auto &date_dir : fs::directory_iterator(carid.path())) {
                std::string date_dir_name = date_dir.path().filename().string();
                if (!fs::is_directory(date_dir)) {
                    continue;
                }

                std::regex pattern("[0-9]{8}$");
                std::smatch regex_match;
                if (std::regex_match(date_dir_name, regex_match, pattern)) {
                    tm tm_result;
                    time_t dir_ts;
                    strptime(date_dir_name.c_str(), "%Y%m%d", &tm_result); // 将字符串转换为tm时间
                    dir_ts = mktime(&tm_result); // 将tm时间转换为秒时间
                    if (ts_now - dir_ts > n_days * 24 * 60 * 60) {
                        MLOG(Icafe, INFO) << "Find matched date: " << date_dir_name;
                        for (auto &log_it : fs::directory_iterator(date_dir.path())) {
                            if (!fs::is_directory(log_it)) {
                                continue;
                            }
                            std::regex pattern_log("[a-zA-Z0-9]+_[0-9]{14}_[a-zA-Z]+_LOG$");
                            std::smatch regex_match;
                            if (std::regex_match(log_it.path().filename().string(), regex_match,
                                    pattern_log)) {
                                MLOG(Icafe, INFO)
                                    << "Delete log: " << log_it.path().filename().string();

                                fs::remove_all(log_it);
                            }
                        }
                    }
                }
            }
        }
        // 3.Delete adb logs.
        if (!fs::exists("./logs")) {
            MLOG(Bag, INFO) << "Adb logs dir not exists !";
            return;
        }
        for (auto &log : fs::directory_iterator("./logs")) {
            if (!fs::exists(log)) {
                continue;
            }
            if (ts_now - fs::last_write_time(log) >
                AsdtContext::Instance()->getAsdtConfig().adb_log_expired_days * 24 * 60 * 60) {
                MLOG(Bag, INFO) << "Delete adb logs name: " << log.path().filename().string();

                MLOG(Bag, INFO) << "Adb logs last update time: "
                                << util::to_timestr(fs::last_write_time(log));
                fs::remove_all(log);
            }
        }
        // 4.Delete expired cutbag
        if ("asd-cloudbag" != util::get_host_name()) {
            if (!fs::exists(AsdtContext::Instance()->getAsdtConfig().cut_bag_dir)) {
                MLOG(Bag, ERROR) << "Cut bag dir not exists !";
                return;
            }
            for (auto &bag_dir :
                fs::directory_iterator(AsdtContext::Instance()->getAsdtConfig().cut_bag_dir)) {
                if (!fs::is_directory(bag_dir)) {
                    continue;
                }
                for (auto &bag_data_dir : fs::directory_iterator(bag_dir)) {
                    MLOG(Bag, INFO) << "bag_data_dir = " << bag_data_dir.path().filename().string();
                    if (!fs::exists(bag_data_dir)) {
                        continue;
                    }
                    if (fs::last_write_time(bag_data_dir) < ts_before_ndays) {
                        MLOG(Bag, INFO)
                            << "Delete cut bag name: " << bag_data_dir.path().filename().string()
                            << "its last write time: "
                            << util::to_timestr(fs::last_write_time(bag_data_dir));
                        fs::remove_all(bag_data_dir);
                    }
                }
            }
        }
    } catch (fs::filesystem_error &e) {
        MLOG(Bag, INFO) << "Delete expired files error: " << e.what();
    } catch (std::exception &other_e) {
        MLOG(Bag, INFO) << "Delete expired files other error: " << other_e.what();
    }
}
