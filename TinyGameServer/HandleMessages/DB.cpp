#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <iostream>

namespace TinyGameServer {



    enum class LoginStatus {
        SUCCESS = 0,
        PASSWORD_INCORRECT = 1,
        NEW_USER = 2,
        SQL_ERROR =3,
    };

    class RankListItem{
    public:
        int client_id;
        int score;
        RankListItem(int id ,int s):client_id(id),score(s){}
    };

    class DB_Mgr: public Singleton<DB_Mgr>
    {
    public:

    LoginStatus verifyRegisterUser(const std::string &username, const std::string &password, int &client_id) {
        try {
            sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> conn(driver->connect("tcp://localhost:3306", "root", "123456"));

            conn->setSchema("game_db");

            // 检查用户是否存在
            std::unique_ptr<sql::PreparedStatement> pstmt(
                    conn->prepareStatement("SELECT client_id FROM users WHERE username = ?")
            );
            pstmt->setString(1, username);

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

            if (res->next()) {
                // 用户存在，验证密码
                client_id = res->getInt("client_id");
                std::cout << "User found with client_id: " << client_id << std::endl;

                pstmt.reset(conn->prepareStatement("SELECT * FROM users WHERE username = ? AND password = ?"));
                pstmt->setString(1, username);
                pstmt->setString(2, password);
                res.reset(pstmt->executeQuery());

                if (res->next()) {
                    std::cout << "Login successful for client_id: " << client_id << std::endl;
                    return LoginStatus::SUCCESS;  // 登录成功
                } else {
                    std::cout << "Password incorrect for username: " << username << std::endl;
                    return LoginStatus::PASSWORD_INCORRECT;  // 密码错误
                }
            } else {
                // 用户不存在，创建新用户
                std::cout << "User not found. Registering new user with username: " << username << std::endl;

                std::unique_ptr<sql::PreparedStatement> insert_stmt(
                        conn->prepareStatement("INSERT INTO users (username, password) VALUES (?, ?)")
                );
                insert_stmt->setString(1, username);
                insert_stmt->setString(2, password);
                insert_stmt->executeUpdate();

                std::cout << "New user registered: " << username << std::endl;
                // 获取新用户的client_id
                std::unique_ptr<sql::PreparedStatement> get_id_stmt(
                        conn->prepareStatement("SELECT client_id FROM users WHERE username = ?")
                );
                get_id_stmt->setString(1, username);
                std::unique_ptr<sql::ResultSet> id_res(get_id_stmt->executeQuery());

                if (id_res->next()) {
                    client_id = id_res->getInt("client_id");
                    std::cout << "New user registered with client_id: " << client_id << std::endl;
                    return LoginStatus::NEW_USER;
                } else {
                    std::cerr << "Failed to retrieve client_id for new user: " << username << std::endl;
                    return LoginStatus::SQL_ERROR;
                }
            }
        } catch (sql::SQLException &e) {
            std::cerr << "SQL error: " << e.what() << std::endl;
            return LoginStatus::SQL_ERROR;
        }
    }

    std::vector<RankListItem> getRankList(){
        std::vector<RankListItem> rankList;
        try {
            sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> conn(driver->connect("tcp://localhost:3306", "root", "123456"));

            conn->setSchema("game_db");

            std::unique_ptr<sql::PreparedStatement> pstmt(
                    conn->prepareStatement("SELECT client_id, score FROM ranklist ORDER BY score DESC LIMIT 3")
            );

            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

            while (res->next()) {
                int client_id = res->getInt("client_id");
                int score = res->getInt("score");
                rankList.emplace_back(client_id, score);
            }
        } catch (sql::SQLException &e) {
            std::cerr << "SQL error: " << e.what() << std::endl;
        }
        return rankList;
    }

    void updateRankScore(int client_id, int score) {
        try {
            sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> conn(driver->connect("tcp://localhost:3306", "root", "123456"));

            conn->setSchema("game_db");

            std::unique_ptr<sql::PreparedStatement> pstmt(
                    conn->prepareStatement("INSERT INTO ranklist (client_id, score) VALUES (?, ?) "
                                           "ON DUPLICATE KEY UPDATE score = GREATEST(score, VALUES(score))")
            );
            pstmt->setInt(1, client_id);
            pstmt->setInt(2, score);
            pstmt->executeUpdate();
        } catch (sql::SQLException &e) {
            std::cerr << "SQL error: " << e.what() << std::endl;
        }
    }


    };

}

