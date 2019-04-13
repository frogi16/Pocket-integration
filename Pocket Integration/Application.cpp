#include "Application.h"
#include <iostream>

Application::Application() noexcept :
	authenticator(keyHolder.pocketKey),
	adder(keyHolder.pocketKey),
	pocketRetriever(keyHolder.pocketKey),
	parser()
{
	srand(time(NULL));
	currentUser.username = "";
	currentUser.accessToken = "";
	updater.setUpdateFrequencyInMinutes(5);
	loadFeedsToWatch();
}

void Application::run()
{
	authenticateConnection();
	std::vector<ArticleRSS> articles = articlesDatabase.loadDatabase();		//info about articles. Title, link, description etc. No actual content

	while (true)
	{
		std::cout << std::endl << "There are " << articles.size() << " new articles!" << std::endl;
		std::cout << "Type \"update\" to check for new articles and \"book\" to create epub and mobi files." << std::endl;
		std::string input;
		std::cin >> input;

		if (input == "update")
		{
			auto newArticles = pocketRetriever.retrieveArticles(currentUser.accessToken);
			articles.insert(articles.end(), newArticles.begin(), newArticles.end());

			newArticles = checkRSS();
			articles.insert(articles.end(), newArticles.begin(), newArticles.end());
			articlesDatabase.saveDatabase(articles);
		}
		else if(input=="book")
		{
			createMobi(articles);
			articles.clear();
			articlesDatabase.saveDatabase(articles);
		}
		else
		{
			std::cout << "Incorrect input" << std::endl;
		}

		//addArticlesToPocket(newArticles);
	}
}

void Application::authenticateConnection()
{
	std::cout << "Authenticating connection" << std::endl;

	if (users.size() > 0)
	{
		currentUser.username = users.begin()->first;			//for now application just uses first user on the list. Later, maybe with GUI, there should be some possibility to choose user.
		currentUser.accessToken = users.begin()->second;
	}
	else
	{
		try
		{
			UserData user = authenticator.authenticate();		//authenticate by connecting to pocket and redirecting user to their website (uses OAuth 2.0)
			users[user.username] = user.accessToken;
			currentUser = user;
		}
		catch (const std::exception &e)
		{
			std::cout << "Pocket authentication failed: " << e.what() << std::endl;
		}
	}
}

std::vector<ArticleRSS> Application::checkRSS()
{
	return updater.checkUpdates();
}

std::vector<ArticleRSS> Application::getArticlesFromPocket()
{
	return std::vector<ArticleRSS>();
}

void Application::addArticlesToPocket(const std::vector<std::string> & urls)
{
	if (urls.size())
		std::cout << "Sending " << urls.size() << " articles to pocket" << std::endl;

	adder.addArticles(urls, currentUser.accessToken);
}

void Application::createMobi(const std::vector<ArticleRSS>& items)
{
	if (items.size())
	{
		std::cout << "Parsing " << items.size() << " articles" << std::endl;

		auto articles = parser.getParsedArticles(items);						//after parsing ParsedArticle contains all informations about article - title, description, full content in string and xml tree simultaneously etc.

		std::cout << "Filtering articles" << std::endl;

		filter.filterArticles(articles);										//filtering out fragments of articles, removing too short and too long ones

		if (articles.size())
		{
			std::cout << "Creating epub from  " << articles.size() << " articles" << std::endl;

			ebookCreator.createEpub(articles);

			std::cout << "Converting to mobi" << std::endl;

			ebookCreator.convertToMobi();
			//ebookCreator.removeEpub();
		}
	}
}

void Application::loadFeedsToWatch()
{
	std::fstream watchedFeeds("watchedFeeds.txt", std::ios::in);

	while (!watchedFeeds.eof())
	{
		std::string feed;
		watchedFeeds >> feed;
		updater.watchFeed(feed);
	}
}

Application::~Application()
{
}
