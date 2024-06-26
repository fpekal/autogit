#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <thread>

#include <iostream>

enum log_level {
	ERROR=0, NORMAL=1, DEBUG=2
};

struct {
	std::string config_loc = "/etc/autogit/autogit.conf";
	log_level log = NORMAL;
} env;

static void log(log_level level, std::string msg) {
	if (level > env.log) return;

	if (level == ERROR) std::cout << "ERROR: ";
	if (level == NORMAL);
	if (level == DEBUG) std::cout << "DEBUG: ";

	std::cout << msg << '\n';
}

static void parse_arguments(int argc, const char* argv[]) {
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];

		if (arg == "--help") {
			log(NORMAL, "autogit automatically commits all changes in a working directory to the git repository and pushes them to remote.");
			log(NORMAL, "");
			log(NORMAL, "{--config,-c} config_file - set config file path - default /etc/autogit/autogit.conf");
			log(NORMAL, "{--log,-l} {silent,normal,verbose} - set log level - default normal");

			exit(0);
		}
		else if (arg == "--config" || arg == "-c") {
			++i;
			if (i >= argc) return;

			env.config_loc = argv[i];
		}
		else if (arg == "--log" || arg == "-l") {
			++i;
			if (i >= argc) return;

			std::string log_type = argv[i];

			if (log_type == "silent") env.log = ERROR;
			else if (log_type == "normal") env.log = NORMAL;
			else if (log_type == "verbose") env.log = DEBUG;
		}
		else {
			std::cout << "Unrecognised argument " << arg << '\n';
		}
	}
}

static void parse_env_var() {
	auto config = std::getenv("AUTOGIT_CONFIG");
	if (config == nullptr) return;

	env.config_loc = config;
}








struct repository {
	std::string location;
	int period; // in minutes
	int left;   // in minutes
	
	bool push = false;
};



static std::vector<repository> read_config() {
	std::vector<repository> out;

	std::ifstream file { env.config_loc };
	if (!file) {
		log(ERROR, "Config file read error");
		exit(-1);
	}

	while (file.peek() != EOF) {
		repository tmp;
		file >> tmp.period;
		file.ignore(1);
		
		std::string push;
		file >> push;
		if (push == "false") tmp.push = false;
		else if (push == "true") tmp.push = true;
		file.ignore(1);

		tmp.left = tmp.period;
		std::getline(file, tmp.location);

		log(DEBUG, "Loaded repository:");
		log(DEBUG, std::string{"Location: "} + tmp.location);
		log(DEBUG, std::string{"Minutes: "} + std::to_string(tmp.period));
		log(DEBUG, std::string{"Push: "} + push);
		log(DEBUG, "");

		out.push_back(tmp);
	}

	log(DEBUG, "Ended loading config");

	return out;
}


static void commit_repository(repository& repo) {
	log(DEBUG, std::string{ "Commiting repository: " } + repo.location);

	//TODO Maybe prettify???
	std::ignore = system((std::string{ "cd " } + repo.location + "; git add . && git commit -m \"commit $(date \"+%Y-%m-%d %H:%M:%S\")\" > /dev/null " + (repo.push ? "&& git push > /dev/null" : "")).c_str());
}

static void minute_passed(std::vector<repository>& repositories) {
	for (auto& repo : repositories) {
		--repo.left;

		if (repo.left <= 0) {
			commit_repository(repo);
			repo.left += repo.period;
		}
	}
}



int main(int argc, const char* argv[]) {
	parse_env_var();
	parse_arguments(argc, argv);

	log(NORMAL, "Autogit started");
	log(DEBUG, std::string{"Selected config location: "} + env.config_loc);

	auto repositories = read_config();

	while (true) {
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(1min);

		minute_passed(repositories);
	}
}
