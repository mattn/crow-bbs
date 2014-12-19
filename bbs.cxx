#include <memory>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include "crow_all.h"

int
main() {
  std::ifstream conf("config.json");
  if (!conf) {
    std::cerr << "config.json not found" << std::endl;
    return 1;
  }
  std::string json = {
    std::istreambuf_iterator<char>(conf),
    std::istreambuf_iterator<char>()};
  crow::json::rvalue config = crow::json::load(json);

  auto driver = sql::mysql::get_mysql_driver_instance();
  auto raw_con = driver->connect(
    (std::string) config["db_host"].s(),
    (std::string) config["db_user"].s(),
    (std::string) config["db_pass"].s());
  auto con = std::unique_ptr<sql::Connection>(raw_con);
  con->setSchema((std::string) config["db_name"].s());

  crow::SimpleApp app;
  crow::mustache::set_base(".");

  CROW_ROUTE(app, "/")
  ([&]{
    auto stmt = std::unique_ptr<sql::PreparedStatement>(
      con->prepareStatement("select * from bbs order by created"));
    auto res = std::unique_ptr<sql::ResultSet>(
	  stmt->executeQuery());
    int n = 0;
    crow::mustache::context ctx;
    while (res->next()) {
      ctx["posts"][n]["id"] = res->getInt("id");
      ctx["posts"][n]["text"] = res->getString("text");
      n++;
    } 
    return crow::mustache::load("bbs.html").render(ctx);
  });

  CROW_ROUTE(app, "/post")
      .methods("POST"_method)
  ([&](const crow::request& req, crow::response& res){
    crow::query_string params(req.body);
    auto stmt = std::unique_ptr<sql::PreparedStatement>(
      con->prepareStatement("insert into bbs(text) values(?)"));
    stmt->setString(1, params.get("text"));
    stmt->executeUpdate();
    res = crow::response(302);
    res.set_header("Location", "/");
    res.end();
  });

  app.port(40081)
    //.multithreaded()
    .run();
}
