#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <queue>
#include <list>
#include <stack>
#include <ctime>
#include <algorithm>
#include <fstream>
using namespace std;
using namespace sf;



struct Task
{
	string title;
	int priority;
	string date;
	bool completed = false;
	 
	Task() : title(""), priority(0), date("") {}
	Task(string t, int p, string d) : title(t), priority(p), date(d) {}
};

struct TaskCompare
{
	bool operator()(Task const& a, Task const& b) const
	{
		return a.priority < b.priority;
	}
};

struct DailyTask
{
	string title;
	string date;
	string status;
	bool completed ;
	string completedTime;
	DailyTask* next;

	DailyTask() : title(""), date(""), completed(false), completedTime(""), next(nullptr) {}
	DailyTask(string t, string d, string s) : title(t), date(d), status(s), completed(false), completedTime(""), next(nullptr){}
};
DailyTask* head = nullptr;

stack<DailyTask*> dailyCompletedStack;

const string TASK_FILE = "tasks.txt";

void saveTasks(priority_queue<Task, vector<Task>, TaskCompare> pq)
{
	ofstream fout(TASK_FILE);
	auto copy = pq;
	while (!copy.empty())
	{
		Task t = copy.top();
		copy.pop();
        fout << t.title << "|" << t.priority << "|" << t.date << "|" << t.completed << "\n";	}
}

priority_queue<Task, vector<Task>, TaskCompare> loadTasks()
{
	priority_queue<Task, vector<Task>, TaskCompare> pq;
	ifstream fin(TASK_FILE);
	string line;
	while (getline(fin, line))
	{
		size_t p1 = line.find('|');
		size_t p2 = line.rfind('|');
		if (p1 == string::npos || p2 == string::npos) continue;
		string title = line.substr(0, p1);
int priority = stoi(line.substr(p1 + 1, p2 - p1 - 1));
size_t p3 = line.rfind('|');
string date = line.substr(p2 + 1, p3 - p2 - 1);
bool completed = line.substr(p3 + 1) == "1";
Task t(title, priority, date);
t.completed = completed;
pq.push(t);
	}

	return pq;
}

const string DAILY_TASK_FILE = "daily_tasks.txt";

void saveDailyTasks(DailyTask* head)
{
	ofstream fout(DAILY_TASK_FILE);
	DailyTask* temp = head;
	while (temp != nullptr)
	{
		fout << temp->title << "|" << temp->date << "|" << temp->status << "|" << temp->completed << "|" << temp->completedTime << "\n";
		temp = temp->next;
	}
}

DailyTask* loadDailyTasks()
{
	ifstream fin(DAILY_TASK_FILE);
	string line;
	DailyTask* head = nullptr;
	DailyTask* tail = nullptr;

	while (getline(fin, line))
	{
		size_t p1 = line.find('|');
		size_t p2 = line.find('|', p1 + 1);
		size_t p3 = line.find('|', p2 + 1);
		size_t p4 = line.find('|', p3 + 1);

		if (p1 == string::npos || p2 == string::npos || p3 == string::npos || p4 == string::npos) continue;

		string title = line.substr(0, p1);
		string date = line.substr(p1 + 1, p2 - p1 - 1);
		string status = line.substr(p2 + 1, p3 - p2 - 1);
		bool completed = line.substr(p3 + p1, p4 - p3 - 1) == "1";
		string completedTime = line.substr(p4 + 1);

		DailyTask* newTask = new DailyTask(title, date, status);
		newTask->completed = completed;
		newTask->completedTime = completedTime;

		if (head == nullptr)
		{
			head = tail = newTask;
		}
		else
		{
			tail->next = newTask;
			tail = newTask;
		}
	}
	return head;
}

struct CompletedTask
{
	string title;
	string date;
	string source;
	string completedTime;
};

stack<CompletedTask> priorityCompletedStack;


void openPriorityTasks();
void openDailyTasks();
void openCompletedTasks();
void openMainMenu();

void openPriorityTasks()
{
	RenderWindow window(VideoMode({ 1400, 900 }), "Priority Tasks");

	Color bgColor(245, 240, 230);
	Color buttonColor(100, 150, 240);
	Color backColor(200, 80, 80);
	Color barColor(150, 200, 250);
	Color popupColor(200, 200, 255);

	Font font;
	if (!font.openFromFile("assets/pixel.ttf"))
	{
		cout << "could not load font\n";
		return;
	}

	RectangleShape addBtn(Vector2f( 270.f, 60.f ));
	addBtn.setPosition({ 30.f, 90.f });
	addBtn.setFillColor(buttonColor);
	addBtn.setOutlineThickness(3.f);
	addBtn.setOutlineColor(Color::Black);

	RectangleShape backBtn(Vector2f( 140.f, 50.f ));
	backBtn.setPosition({ 1230.f, 30.f });
	backBtn.setFillColor(backColor);
	backBtn.setOutlineThickness(3.f);
	backBtn.setOutlineColor(Color::Black);

	Text addText(font);
	addText.setString("Add Task");
	addText.setCharacterSize(26);
	addText.setFillColor(Color::White);
	addText.setPosition({ 60.f, 105.f });

	Text backText(font);
	backText.setString("Back");
	backText.setCharacterSize(22);
	backText.setFillColor(Color::White);
	backText.setPosition({ 1265.f, 40.f });
	

	Texture bgTex;
	if (!bgTex.loadFromFile("sea.png"))
	{
		cout << "Failed to load to sea.png\n";
		return;
	}
	bgTex.setRepeated(true);
	Sprite bg(bgTex);
	bg.setTextureRect(IntRect({ 0, 0 }, { 1400 * 3, 900 }));

	float bgOffset = 0.f;
	Clock clock;

	auto pq = loadTasks();

	float scrollOffset = 0.f;
	float maxScroll = 0.f;

	bool showPopup = false, taskClickPopup = false;
	string inputTitle, inputPriority, inputDate;
	int selectedTaskIndex = -1;

	enum Focus {NONE, TITLE, PRIORITY, DATE};
	Focus focused = NONE ;

	RectangleShape popup({ 700.f, 320.f });
	popup.setFillColor(popupColor);
	popup.setOutlineThickness(2);
	popup.setOutlineColor(Color::Black);
	popup.setPosition({ 350.f, 220.f });

	RectangleShape titleBox({ 350.f, 35.f });
	RectangleShape priorityBox({ 350.f, 35.f });
	RectangleShape dateBox({ 350.f, 35.f });
	titleBox.setFillColor(Color::White);
	priorityBox.setFillColor(Color::White);
	dateBox.setFillColor(Color::White);

	RectangleShape confirmPopup({ 360.f, 150.f });
	confirmPopup.setFillColor(Color(220, 220, 220));
	confirmPopup.setOutlineThickness(2);
	confirmPopup.setOutlineColor(Color::Black);
	confirmPopup.setPosition({ 520.f, 340.f });

	while (window.isOpen())
	{
		float dt = clock.restart().asSeconds();
		bgOffset -= 80.f * dt;

		IntRect rect = bg.getTextureRect();
		rect.position.x = static_cast<int>(bgOffset);
		bg.setTextureRect(rect);

		if (bgOffset <= -1400)
			bgOffset = 0;

		while (auto event = window.pollEvent())
		{
			if (event->is<Event::Closed>())
			{
				window.close();
				return;
			}

			if (auto mouse = event->getIf<Event::MouseButtonPressed>())
			{
				if (mouse->button == Mouse::Button::Left)
				{
					Vector2f click(
						static_cast<float>(mouse->position.x),
						static_cast<float>(mouse->position.y)
					);

					if (backBtn.getGlobalBounds().contains(click))
					{
						saveTasks(pq);
						window.close();
						openMainMenu();
						return;
					}

					if (addBtn.getGlobalBounds().contains(click))
					{
						showPopup = true;
						taskClickPopup = false;
						focused = TITLE;
						inputTitle.clear();
						inputPriority.clear();
						inputDate.clear();
					}

					if (showPopup)
					{
						if (titleBox.getGlobalBounds().contains(click))
							focused = TITLE;
						else if (priorityBox.getGlobalBounds().contains(click))
							focused = PRIORITY;
						else if (dateBox.getGlobalBounds().contains(click))
							focused = DATE;
						else 
							focused = NONE;
					}

					if (!showPopup && !taskClickPopup)
					{
						vector<Task> tasks;
						auto copy = pq;
						while (!copy.empty())
						{
							tasks.push_back(copy.top());
							copy.pop();
						}
						reverse(tasks.begin(), tasks.end());

						float yStart = 170.f, barH = 55.f, spacing = 12.f;
						for (size_t i = 0; i < tasks.size(); i++)
						{
							float y = yStart + i * (barH + spacing) + scrollOffset;
							FloatRect tickBox({ 100 + 1160.f, y + 12.f }, { 30.f, 30.f });
							if (tickBox.contains(click))
							{
								priority_queue<Task, vector<Task>, TaskCompare> newPQ;
								while (!pq.empty())
								{
									Task t = pq.top(); pq.pop();
									if (t.title == tasks[i].title && t.date == tasks[i].date)
									{

										if (!t.completed)
										{
											t.completed = true;

											CompletedTask ct;
											ct.title = t.title;
											ct.source = "Priority";
											ct.completedTime = t.date;
											priorityCompletedStack.push(ct);
										}
									}
									newPQ.push(t);

								}
								pq = newPQ;
								break;
							}


							FloatRect barRect({ 100.f, y }, {1200.f, barH});
							if (barRect.contains(click))
							{
								selectedTaskIndex = i;
								taskClickPopup = true;
								break;
							}
						}
					}
				}
			}

			if (auto wheel = event->getIf<Event::MouseWheelScrolled>())
			{
				scrollOffset -= wheel->delta * 30.f;
				scrollOffset = min(0.f, scrollOffset);
				scrollOffset = max(-maxScroll, scrollOffset);
			}

			if (auto text = event->getIf<Event::TextEntered>())
			{
				if (!showPopup) continue;

				if (focused == TITLE && text->unicode >= 32 && text->unicode <= 126)
					inputTitle += static_cast<char>(text->unicode);

				if (focused == PRIORITY && text->unicode >= '0' && text->unicode <= '9')
					inputPriority += static_cast<char>(text->unicode);

				if (focused == DATE && text->unicode >= 32 && text->unicode <= 126)
					inputDate += static_cast<char>(text->unicode);
			}

			if (auto key = event->getIf<Event::KeyPressed>())
			{
				if (!showPopup) continue;

				if (key->code == Keyboard::Key::Backspace)
				{
					if (focused == TITLE && !inputTitle.empty())
						inputTitle.pop_back();
					else if (focused == PRIORITY && !inputPriority.empty())
						inputPriority.pop_back();
					else if (focused == DATE && !inputDate.empty())
						inputDate.pop_back();

				}

				if (key->code == Keyboard::Key::Tab)
				{
					if (focused == TITLE)
						focused = PRIORITY;
					else if (focused == PRIORITY)
						focused = DATE;
					else if (focused == DATE)
						focused = TITLE;
				}

				if (key->code == Keyboard::Key::Enter)
				{
					if (!inputTitle.empty() && !inputPriority.empty() && !inputDate.empty())
					{
						pq.push({ inputTitle, stoi(inputPriority), inputDate });
						showPopup = false;
						focused = NONE;
					}
				}

				if (key->code == Keyboard::Key::Escape)
				{
					showPopup = false;
					focused = NONE;
				}
			}
		}

		window.clear(bgColor);
		window.draw(bg);
		window.draw(addBtn);
		window.draw(backBtn);
		window.draw(addText);
		window.draw(backText);

		vector <Task>tasks;
		auto copy = pq;
		while (!copy.empty()) { tasks.push_back(copy.top()); copy.pop(); }
		reverse(tasks.begin(), tasks.end());

		float yStart = 170.f;
		float barH = 55.f;
		float spacing = 12.f;

		for (size_t i = 0; i < tasks.size(); i++)
		{
			float y = yStart + i * (barH + spacing) + scrollOffset;
if (tasks[i].completed) continue;
if (y < -barH || y > 900) continue;
			RectangleShape bar({ 1200.f, barH });
			bar.setPosition({ 100.f, y });
		    Color taskColor;
if (tasks[i].priority >= 9) taskColor = Color(220, 80, 80);
else if (tasks[i].priority >= 6) taskColor = Color(230, 150, 50);
else if (tasks[i].priority >= 3) taskColor = Color(230, 200, 50);
else taskColor = Color(80, 180, 80);
bar.setFillColor(taskColor);
bar.setOutlineThickness(2);
bar.setOutlineColor(Color::Black);

			Text t(font);
			t.setCharacterSize(22);
			t.setFillColor(Color::Black);
string label;
if (tasks[i].priority >= 9) label = "[URGENT]";
else if (tasks[i].priority >= 6) label = "[HIGH]";
else if (tasks[i].priority >= 3) label = "[MEDIUM]";
else label = "[LOW]";
t.setString(label + " " + tasks[i].title + "    (Due: " + tasks[i].date + ")");			t.setPosition({ 120.f, y + 14.f });

			window.draw(bar);
			window.draw(t);

			RectangleShape tickBox({ 30.f, 30.f });
			tickBox.setPosition({ 100.f + 1160.f, y + 12.f });
			tickBox.setOutlineThickness(2);
			tickBox.setOutlineColor(Color::Black);

			if (tasks[i].completed)
			{
				tickBox.setFillColor(Color::Green);
				window.draw(tickBox);

				Text tick(font);
				tick.setString("c");
				tick.setCharacterSize(22);
				tick.setFillColor(Color::White);
				tick.setPosition(tickBox.getPosition() + Vector2f(4, -2));
				window.draw(tick);
			}
			else
			{
				tickBox.setFillColor(Color::White);
				window.draw(tickBox);

			}

		
		}

		maxScroll = max(0.f, tasks.size() * (barH + spacing) - 700.f);

		if (showPopup)
		{
			window.draw(popup);

			titleBox.setPosition(popup.getPosition() + Vector2f(240, 40));
			titleBox.setOutlineThickness(focused == TITLE ? 2.f : 0.f);
			titleBox.setOutlineColor(Color::Black);
			window.draw(titleBox);

			priorityBox.setPosition(popup.getPosition() + Vector2f(240, 130));
			priorityBox.setOutlineThickness(focused == PRIORITY ? 2.f : 0.f);
			priorityBox.setOutlineColor(Color::Black);
			window.draw(priorityBox);

			dateBox.setPosition(popup.getPosition() + Vector2f(240, 220));
			dateBox.setOutlineThickness(focused == DATE ? 2.f : 0.f);
			dateBox.setOutlineColor(Color::Black);
			window.draw(dateBox);

			Text l1(font);
			l1.setString("Task");
			l1.setCharacterSize(22);
			l1.setPosition(popup.getPosition() + Vector2f(30, 50));
			Text l2(font);
			l2.setString("Priority");
			l2.setCharacterSize(22);
			l2.setPosition(popup.getPosition() + Vector2f(30, 140));
			Text l3(font);
			l3.setString("Due Date");
			l3.setCharacterSize(22);
			l3.setPosition(popup.getPosition() + Vector2f(30, 230));

			Text i1(font);
			i1.setFillColor(Color::Black);
			i1.setString(inputTitle);
			i1.setCharacterSize(22);
			i1.setPosition(titleBox.getPosition() + Vector2f(8, 6));
			Text i2(font);
			i2.setFillColor(Color::Black);
			i2.setString(inputPriority);
			i2.setCharacterSize(22);
			i2.setPosition(priorityBox.getPosition() + Vector2f(8, 6));
			Text i3(font);
			i3.setFillColor(Color::Black);
			i3.setString(inputDate);
			i3.setCharacterSize(22);
			i3.setPosition(dateBox.getPosition() + Vector2f(8, 6));

			window.draw(l1); window.draw(l2); window.draw(l3);
			window.draw(i1); window.draw(i2); window.draw(i3);
	

		}



		if (taskClickPopup && selectedTaskIndex >= 0 && selectedTaskIndex < tasks.size())
		{
			window.draw(confirmPopup);
			Text msg(font);
			msg.setFillColor(Color::Blue);
			msg.setString("Keep or Remove?");
			msg.setCharacterSize(18);
			msg.setPosition(confirmPopup.getPosition() + Vector2f(50, 20));
			window.draw(msg);

			RectangleShape keepBtn(Vector2f(100, 40));
			keepBtn.setPosition(confirmPopup.getPosition() + Vector2f(40, 60));
			keepBtn.setFillColor(Color(100, 200, 100));
			keepBtn.setOutlineThickness(2);
			keepBtn.setOutlineColor(Color::Black);
			keepBtn.setSize(Vector2f(130, 50));
			window.draw(keepBtn);

			RectangleShape removeBtn(Vector2f(100, 40));
			removeBtn.setPosition(confirmPopup.getPosition() + Vector2f(190, 60));
			removeBtn.setFillColor(Color(200, 100, 100));
			removeBtn.setOutlineThickness(2);
			removeBtn.setOutlineColor(Color::Black);
			removeBtn.setSize(Vector2f(130, 50));
			window.draw(removeBtn);

			Text keepText(font);
			keepText.setFillColor(Color::White);
			keepText.setString("Keep");
			keepText.setCharacterSize(20);
			keepText.setPosition(keepBtn.getPosition() + Vector2f(19, 10));
			window.draw(keepText);

			Text removeText(font);
			removeText.setFillColor(Color::White);
			removeText.setString("Remove");
			removeText.setCharacterSize(20);
			removeText.setFillColor(Color::White);
			removeText.setPosition(removeBtn.getPosition() + Vector2f(9, 10));
			window.draw(removeText);

			static bool clicked = false;

			if (Mouse::isButtonPressed(Mouse::Button::Left) && !clicked)
			{
				clicked = true;
				Vector2f mousePos = static_cast<Vector2f>(Mouse::getPosition(window));
				if (keepBtn.getGlobalBounds().contains(mousePos))
				{
					taskClickPopup = false;
					selectedTaskIndex = -1;
				}
				else if (removeBtn.getGlobalBounds().contains(mousePos))
				{
					priority_queue<Task, vector<Task>, TaskCompare> newPQ;
					for (size_t i = 0; i < tasks.size(); i++)
						if (i != selectedTaskIndex)
							newPQ.push(tasks[i]);
					pq = newPQ;
					saveTasks(pq);
					taskClickPopup = false;
					selectedTaskIndex = -1;
				}
			}

			if (!Mouse::isButtonPressed(Mouse::Button::Left))
				clicked = false;
		}

		window.display();
	}


}

void addDailyTask(string t, string d, string s)
{
	DailyTask* newTask = new DailyTask{ t, d, s };
	if (!head)
		head = newTask;
	else
	{
		DailyTask* temp = head;
		while (temp->next) temp = temp->next;
		temp->next = newTask;
	}
}

void removeDailyTask(DailyTask* taskToRemove)
{
	if (!head || !taskToRemove) return;
	if (head == taskToRemove)
	{
		head = head->next;
		delete taskToRemove;
		return;
	}
	DailyTask* temp = head;
	while (temp->next && temp->next != taskToRemove)
		temp = temp->next;
	if (temp->next)
	{
		temp->next = taskToRemove->next;
		delete taskToRemove;
	}
}

void openDailyTasks()
{
	RenderWindow window(VideoMode({ 1400, 900 }), "Daily Tasks");
	window.setKeyRepeatEnabled(true);
	Color bgColor(245, 240, 230);
	Color boxColor(150, 200, 250);
	Color buttonColor(100, 150, 240);
	Color backColor(200, 80, 80);
	Color popupColor(200, 200, 255);

	Font font;
	if (!font.openFromFile("assets/pixel.ttf"))
	{
		cout << "Could not load font/n";
		return;
	}

	Texture bgTexture;
	if (!bgTexture.loadFromFile("sea.png"))
	{
		cout << "could not load background\n";
		return;
	}
	bgTexture.setRepeated(true);
	Sprite bgSprite(bgTexture);
	bgSprite.setTextureRect(IntRect({ 0,0 }, { 1400 * 3, 900 }));

	float bgOffset = 0.f;
	Clock clock;

	head = loadDailyTasks();

	RectangleShape backBtn({ 140.f, 50.f });
	backBtn.setPosition({ 1230.f, 30.f });
	backBtn.setFillColor(backColor);
	backBtn.setOutlineThickness(3.f);
	backBtn.setOutlineColor(Color::Black);

	Text backText(font);
	backText.setString("Back");
	backText.setCharacterSize(22);
	backText.setFillColor(Color::White);
	backText.setPosition({ 1265.f, 40.f });

	RectangleShape addBtn({ 270.f, 60.f });
	addBtn.setPosition({ 30.f, 90.f });
	addBtn.setFillColor(buttonColor);
	addBtn.setOutlineThickness(3.f);
	addBtn.setOutlineColor(Color::Black);

	Text addText(font);
	addText.setString("Add Task");
	addText.setCharacterSize(26);
	addText.setFillColor(Color::White);
	addText.setPosition({ 60.f, 105.f });

	RectangleShape addPopup({ 700.f, 380.f });
	addPopup.setFillColor(popupColor);
	addPopup.setOutlineThickness(2);
	addPopup.setOutlineColor(Color::Black);
	addPopup.setPosition({ 350.f, 200.f });

	RectangleShape keepRemovePopup({ 420.f, 220.f });
	keepRemovePopup.setFillColor(Color(220, 220, 220));
	keepRemovePopup.setOutlineThickness(2);
	keepRemovePopup.setOutlineColor(Color::Black);
	keepRemovePopup.setPosition({ 490.f, 300.f });

	RectangleShape titleBox({ 350.f, 35.f });
	RectangleShape dateBox({ 350.f, 35.f });
	titleBox.setFillColor(Color::White);
	dateBox.setFillColor(Color::White);

	string inputTitle, inputDate;
	string selectedStatus = "Today";
	string updateSelectedStatus = "Today";

	vector<string> statusOptions = { "Today", "Tomorrow", "This Week", "Someday" };
	vector<Color> statusColors = {
		Color(220, 80, 80),
		Color(230, 150, 50),
		Color(80, 160, 80),
		Color(120, 120, 200)
	};

	enum Focus { NONE, TITLE, DATE };
	Focus focused = NONE;

	bool showAddPopup = false;
	bool showKeepRemove = false;

	DailyTask* selectedTask = nullptr;

	float startX = 50.f, startY = 170.f;
	float boxSize = 300.f;
	float spacingX = 30.f, spacingY = 30.f;

	float scrollOffset = 0.f, maxScroll = 0.f;

	while (window.isOpen())
	{
		float dt = clock.restart().asSeconds();

		bgOffset -= 80.f * dt;
		IntRect rect = bgSprite.getTextureRect();
		rect.position.x = static_cast<int>(bgOffset);
		bgSprite.setTextureRect(rect);
		if (bgOffset <= -1400)
			bgOffset = 0;

		while (auto event = window.pollEvent())
		{
			if (event->is<Event::Closed>())
				window.close();

			if (auto mouse = event->getIf<Event::MouseButtonPressed>())
			{
				if (mouse->button == Mouse::Button::Left)
				{
					Vector2f click(static_cast<float>(mouse->position.x),
						static_cast<float>(mouse->position.y));

					if (backBtn.getGlobalBounds().contains(click))
					{
						saveDailyTasks(head);
						window.close();
						openMainMenu();
						return;
					}

					if (addBtn.getGlobalBounds().contains(click) && !showKeepRemove)
					{
						showAddPopup = true;
						focused = TITLE;
						inputTitle.clear();
						inputDate.clear();
						selectedStatus = "Today";
					}

					if (showAddPopup)
					{
						if (titleBox.getGlobalBounds().contains(click))
							focused = TITLE;
						else if (dateBox.getGlobalBounds().contains(click))
							focused = DATE;

						// status option buttons in add popup
						for (int i = 0; i < 4; i++)
						{
							FloatRect btnRect(
								addPopup.getPosition() + Vector2f(240 + i * 80.f, 220),
								{ 70.f, 35.f }
							);
							if (btnRect.contains(click))
								selectedStatus = statusOptions[i];
						}

						// confirm add
						FloatRect confirmRect(
							addPopup.getPosition() + Vector2f(220, 310),
							{ 200.f, 45.f }
						);
						if (confirmRect.contains(click))
						{
							if (!inputTitle.empty() && !inputDate.empty())
							{
								addDailyTask(inputTitle, inputDate, selectedStatus);
								inputTitle.clear();
								inputDate.clear();
								selectedStatus = "Today";
								showAddPopup = false;
								focused = NONE;
							}
						}

						// cancel
						FloatRect cancelRect(
							addPopup.getPosition() + Vector2f(30, 310),
							{ 150.f, 45.f }
						);
						if (cancelRect.contains(click))
						{
							showAddPopup = false;
							focused = NONE;
						}
					}

					if (!showAddPopup && !showKeepRemove)
					{
						DailyTask* temp = head;
						int idx = 0;

						while (temp)
						{
							int col = idx % 3;
							int row = idx / 3;

							Vector2f boxPos(
								startX + col * (boxSize + spacingX),
								startY + row * (boxSize + spacingY) - scrollOffset
							);

							FloatRect tickRect(
								boxPos + Vector2f(boxSize - 45.f, 15.f),
								{ 28.f, 28.f }
							);

							if (tickRect.contains(click))
							{
								if (!temp->completed)
								{
									temp->completed = true;
									temp->completedTime = temp->date;
									dailyCompletedStack.push(temp);
								}
								break;
							}

							FloatRect boxRect(boxPos, { boxSize, boxSize });
							if (boxRect.contains(click))
							{
								selectedTask = temp;
								showKeepRemove = true;
								updateSelectedStatus = temp->status;
								break;
							}
							temp = temp->next;
							idx++;
						}
					}

					if (showKeepRemove && selectedTask)
					{
						// update status buttons in keep/remove popup
						for (int i = 0; i < 4; i++)
						{
							FloatRect btnRect(
								keepRemovePopup.getPosition() + Vector2f(10 + i * 100.f, 80),
								{ 90.f, 35.f }
							);
							if (btnRect.contains(click))
								updateSelectedStatus = statusOptions[i];
						}

						FloatRect keepBtnRect(keepRemovePopup.getPosition() + Vector2f(40, 140), Vector2f(150, 50));
						if (keepBtnRect.contains(click))
						{
							selectedTask->status = updateSelectedStatus;
							selectedTask = nullptr;
							showKeepRemove = false;
						}

						FloatRect removeBtnRect(keepRemovePopup.getPosition() + Vector2f(220, 140), Vector2f(150, 50));
						if (removeBtnRect.contains(click))
						{
							DailyTask* toDelete = selectedTask;
							selectedTask = nullptr;
							showKeepRemove = false;
							removeDailyTask(toDelete);
						}
					}
				}
			}

			if (auto wheel = event->getIf<Event::MouseWheelScrolled>())
			{
				scrollOffset -= wheel->delta * 30.f;
				int taskCount = 0;
				DailyTask* tmp = head;
				while (tmp) { taskCount++; tmp = tmp->next; }
				int totalRows = (taskCount + 2) / 3;
				int totalHeight = startY + totalRows * (boxSize + spacingY);
				maxScroll = max(0.f, totalHeight - 900.f);
				scrollOffset = max(-maxScroll, min(0.f, scrollOffset));
			}

			if (auto text = event->getIf<Event::TextEntered>())
			{
				if (showAddPopup && text->unicode >= 32 && text->unicode <= 126)
				{
					char c = static_cast<char>(text->unicode);
					if (focused == TITLE) inputTitle += c;
					else if (focused == DATE) inputDate += c;
				}
			}

			if (auto key = event->getIf<Event::KeyPressed>())
			{
				if (showAddPopup)
				{
					if (key->code == Keyboard::Key::Backspace)
					{
						if (focused == TITLE && !inputTitle.empty()) inputTitle.pop_back();
						else if (focused == DATE && !inputDate.empty()) inputDate.pop_back();
					}
					else if (key->code == Keyboard::Key::Tab)
					{
						if (focused == TITLE) focused = DATE;
						else if (focused == DATE) focused = TITLE;
					}
					else if (key->code == Keyboard::Key::Escape)
					{
						showAddPopup = false;
						focused = NONE;
					}
				}
			}
		}

		window.clear(bgColor);
		window.draw(bgSprite);
		window.draw(backBtn);
		window.draw(backText);
		window.draw(addBtn);
		window.draw(addText);

		DailyTask* temp = head;
		int idx = 0;

		while (temp)
		{
			int col = idx % 3;
			int row = idx / 3;

			Vector2f boxPos(
				startX + col * (boxSize + spacingX),
				startY + row * (boxSize + spacingY) - scrollOffset
			);

			RectangleShape box(Vector2f(boxSize, boxSize));
			box.setPosition(boxPos);
			box.setFillColor(boxColor);
			box.setOutlineThickness(2.f);
			box.setOutlineColor(Color::Black);
			window.draw(box);

			// status color bar at top of card
			Color cardBarColor = statusColors[0];
			for (int i = 0; i < 4; i++)
				if (temp->status == statusOptions[i])
					cardBarColor = statusColors[i];

			RectangleShape statusBar({ boxSize, 12.f });
			statusBar.setPosition(boxPos);
			statusBar.setFillColor(cardBarColor);
			window.draw(statusBar);

			RectangleShape tickBox({ 28.f, 28.f });
			tickBox.setPosition(boxPos + Vector2f(boxSize - 45.f, 15.f));
			tickBox.setOutlineThickness(2.f);
			tickBox.setOutlineColor(Color::Black);
			tickBox.setFillColor(temp->completed ? Color(100, 200, 100) : Color::White);
			window.draw(tickBox);

			if (temp->completed)
			{
				Text tick(font);
				tick.setString("c");
				tick.setCharacterSize(26);
				tick.setFillColor(Color::Black);
				tick.setPosition(tickBox.getPosition() + Vector2f(6.f, -2.f));
				window.draw(tick);
			}

			Text t(font);
			t.setCharacterSize(22);
			t.setFillColor(Color::White);

			t.setString("Title: " + temp->title);
			t.setPosition(boxPos + Vector2f(15, 30));
			window.draw(t);

			t.setString("Date: " + temp->date);
			t.setPosition(boxPos + Vector2f(15, 70));
			window.draw(t);

			// status badge
			Color badgeColor = statusColors[0];
			for (int i = 0; i < 4; i++)
				if (temp->status == statusOptions[i])
					badgeColor = statusColors[i];

			RectangleShape badge({ 160.f, 32.f });
			badge.setPosition(boxPos + Vector2f(15, 115));
			badge.setFillColor(badgeColor);
			badge.setOutlineThickness(1);
			badge.setOutlineColor(Color::Black);
			window.draw(badge);

			Text statusTxt(font);
			statusTxt.setString(temp->status);
			statusTxt.setCharacterSize(18);
			statusTxt.setFillColor(Color::White);
			statusTxt.setPosition(boxPos + Vector2f(20, 120));
			window.draw(statusTxt);

			temp = temp->next;
			idx++;
		}

		if (showAddPopup)
		{
			window.draw(addPopup);

			titleBox.setPosition(addPopup.getPosition() + Vector2f(240, 40));
			dateBox.setPosition(addPopup.getPosition() + Vector2f(240, 130));

			titleBox.setOutlineThickness(focused == TITLE ? 2.f : 1.f);
			titleBox.setOutlineColor(Color::Black);
			dateBox.setOutlineThickness(focused == DATE ? 2.f : 1.f);
			dateBox.setOutlineColor(Color::Black);

			window.draw(titleBox);
			window.draw(dateBox);

			Text l(font);
			l.setCharacterSize(22);
			l.setFillColor(Color::Black);

			l.setString("Title");
			l.setPosition(addPopup.getPosition() + Vector2f(30, 50));
			window.draw(l);

			l.setString("Date");
			l.setPosition(addPopup.getPosition() + Vector2f(30, 140));
			window.draw(l);

			l.setString("Deadline:");
			l.setCharacterSize(20);
			l.setPosition(addPopup.getPosition() + Vector2f(30, 228));
			window.draw(l);

			Text in(font);
			in.setCharacterSize(22);
			in.setFillColor(Color::Black);
			in.setString(inputTitle);
			in.setPosition(titleBox.getPosition() + Vector2f(8, 6));
			window.draw(in);

			in.setString(inputDate);
			in.setPosition(dateBox.getPosition() + Vector2f(8, 6));
			window.draw(in);

			// status option buttons
			for (int i = 0; i < 4; i++)
			{
				RectangleShape optBtn({ 70.f, 35.f });
				optBtn.setPosition(addPopup.getPosition() + Vector2f(240 + i * 80.f, 220));
				optBtn.setFillColor(selectedStatus == statusOptions[i] ? statusColors[i] : Color(180, 180, 180));
				optBtn.setOutlineThickness(2);
				optBtn.setOutlineColor(selectedStatus == statusOptions[i] ? Color::Black : Color::Transparent);
				window.draw(optBtn);

				Text optTxt(font);
				optTxt.setString(statusOptions[i].substr(0, 4));
				optTxt.setCharacterSize(14);
				optTxt.setFillColor(Color::White);
				optTxt.setPosition(optBtn.getPosition() + Vector2f(5, 8));
				window.draw(optTxt);
			}

			// confirm button
			RectangleShape confirmBtn({ 200.f, 45.f });
			confirmBtn.setPosition(addPopup.getPosition() + Vector2f(220, 310));
			confirmBtn.setFillColor(Color(80, 160, 80));
			confirmBtn.setOutlineThickness(2);
			confirmBtn.setOutlineColor(Color::Black);
			window.draw(confirmBtn);

			Text confirmTxt(font);
			confirmTxt.setString("Add Task");
			confirmTxt.setCharacterSize(20);
			confirmTxt.setFillColor(Color::White);
			confirmTxt.setPosition(confirmBtn.getPosition() + Vector2f(35, 10));
			window.draw(confirmTxt);

			// cancel button
			RectangleShape cancelBtn({ 150.f, 45.f });
			cancelBtn.setPosition(addPopup.getPosition() + Vector2f(30, 310));
			cancelBtn.setFillColor(Color(200, 80, 80));
			cancelBtn.setOutlineThickness(2);
			cancelBtn.setOutlineColor(Color::Black);
			window.draw(cancelBtn);

			Text cancelTxt(font);
			cancelTxt.setString("Cancel");
			cancelTxt.setCharacterSize(20);
			cancelTxt.setFillColor(Color::White);
			cancelTxt.setPosition(cancelBtn.getPosition() + Vector2f(25, 10));
			window.draw(cancelTxt);
		}

		if (showKeepRemove && selectedTask)
		{
			window.draw(keepRemovePopup);

			Text msg(font);
			msg.setString("Change Deadline or Remove?");
			msg.setPosition(keepRemovePopup.getPosition() + Vector2f(30, 20));
			msg.setCharacterSize(16);
			msg.setFillColor(Color::Black);
			window.draw(msg);

			// status option buttons in popup
			for (int i = 0; i < 4; i++)
			{
				RectangleShape optBtn({ 90.f, 35.f });
				optBtn.setPosition(keepRemovePopup.getPosition() + Vector2f(10 + i * 100.f, 80));
				optBtn.setFillColor(updateSelectedStatus == statusOptions[i] ? statusColors[i] : Color(180, 180, 180));
				optBtn.setOutlineThickness(2);
				optBtn.setOutlineColor(updateSelectedStatus == statusOptions[i] ? Color::Black : Color::Transparent);
				window.draw(optBtn);

				Text optTxt(font);
				optTxt.setString(statusOptions[i].substr(0, 5));
				optTxt.setCharacterSize(14);
				optTxt.setFillColor(Color::White);
				optTxt.setPosition(optBtn.getPosition() + Vector2f(5, 8));
				window.draw(optTxt);
			}

			RectangleShape keepBtn({ 150, 50 });
			keepBtn.setPosition(keepRemovePopup.getPosition() + Vector2f(40, 140));
			keepBtn.setFillColor(Color(100, 200, 100));
			keepBtn.setOutlineThickness(2);
			keepBtn.setOutlineColor(Color::Black);
			window.draw(keepBtn);

			RectangleShape removeBtn({ 150, 50 });
			removeBtn.setPosition(keepRemovePopup.getPosition() + Vector2f(220, 140));
			removeBtn.setFillColor(Color(200, 100, 100));
			removeBtn.setOutlineThickness(2);
			removeBtn.setOutlineColor(Color::Black);
			window.draw(removeBtn);

			Text keepText(font);
			keepText.setString("Save");
			keepText.setPosition(keepBtn.getPosition() + Vector2f(40, 10));
			keepText.setCharacterSize(20);
			keepText.setFillColor(Color::White);
			window.draw(keepText);

			Text removeText(font);
			removeText.setString("Remove");
			removeText.setPosition(removeBtn.getPosition() + Vector2f(25, 10));
			removeText.setCharacterSize(20);
			removeText.setFillColor(Color::White);
			window.draw(removeText);
		}

		window.display();
	}
}

void openCompletedTasks()
{
	RenderWindow window(VideoMode({ 1400, 900 }), "Completed Tasks");

	Color bgColor(245, 240, 230);
	Color barColor(180, 220, 180);
	Color backColor(200, 80, 80);

	Font font;
	if (!font.openFromFile("assets/pixel.ttf"))
	{
		cout << "Could not load font/n";
		return;
	}

	RectangleShape backBtn({ 140.f, 50.f });
	backBtn.setPosition({ 1230.f, 30.f });
	backBtn.setFillColor(backColor);
	backBtn.setOutlineThickness(3.f);
	backBtn.setOutlineColor(Color::Black);

	Text backText(font);
	backText.setString("Back");
	backText.setCharacterSize(22);
	backText.setFillColor(Color::White);
	backText.setPosition({ 1265.f, 40.f });

	Texture bgTexture;
	if (!bgTexture.loadFromFile("sea.png"))
	{
		cout << "could not load background/n";
		return;
	}

	bgTexture.setRepeated(true);

	Sprite bgSprite(bgTexture);
	bgSprite.setTextureRect(IntRect({ 0,0 }, { 1400 * 3, 900 }));

	float bgOffset = 0.f;
	Clock clock;
	float bgSpeed = 80.f;

	float scrollOffset = 0.f;
	float maxScroll = 0.f;

	bool showTopPopup = false;
	static bool clicked = false;
	float barH = 80.f;

	RectangleShape confirmPopup({ 360.f, 150.f });
	confirmPopup.setFillColor(Color(220, 220, 220));
	confirmPopup.setOutlineThickness(2);
	confirmPopup.setOutlineColor(Color::Black);
	confirmPopup.setPosition({ 520.f, 340.f });

	while (window.isOpen())
	{
		float dt = clock.restart().asSeconds();
		bgOffset -= bgSpeed * dt;
		if (bgOffset <= -1400)
			bgOffset = 0;

		IntRect rect = bgSprite.getTextureRect();
		rect.position.x = static_cast<int>(bgOffset);
		bgSprite.setTextureRect(rect);

		while (auto event = window.pollEvent())
		{
			if (event->is<Event::Closed>())
				window.close();

			if (auto mouse = event->getIf<Event::MouseButtonPressed>())
			{
				if (mouse->button == Mouse::Button::Left)
				{
					Vector2f click(
						static_cast<float>(mouse->position.x),
						static_cast<float>(mouse->position.y)
					);

					if (backBtn.getGlobalBounds().contains(click))
					{
						window.close();
						openMainMenu();
						return;
					}

					if (!dailyCompletedStack.empty() || !priorityCompletedStack.empty())
					{
						CompletedTask topTask;
						bool fromPriority = false;

						if (!priorityCompletedStack.empty())
						{
							topTask = priorityCompletedStack.top();
							fromPriority = true;
						}
						else
						{
							DailyTask* t = dailyCompletedStack.top();
							topTask.title = t->title;
							topTask.source = "Daily Tasks";
							topTask.completedTime = t->completedTime;
						}

						FloatRect topBar({ 100.f, 120.f + scrollOffset }, { 1200.f, 80.f });
						if (topBar.contains(click))
							showTopPopup = true;
					}

				}
			}

			if (auto wheel = event->getIf<Event::MouseWheelScrolled>())
			{
				scrollOffset -= wheel->delta * 30.f;
				scrollOffset = min(0.f, scrollOffset);
				scrollOffset = max(-maxScroll, scrollOffset);
			}
		}

		window.clear(bgColor);
		window.draw(bgSprite);
		window.draw(backBtn);
		window.draw(backText);

		stack<CompletedTask> displayStack;


		stack<CompletedTask> tempPriority;
		stack<CompletedTask> s1 = priorityCompletedStack;

		while (!s1.empty())
		{
			tempPriority.push(s1.top());
			s1.pop();
		}
		while (!tempPriority.empty())
		{
			displayStack.push(tempPriority.top());
			tempPriority.pop();
		}

		stack<DailyTask*> tempDaily;
		stack<DailyTask*> s2 = dailyCompletedStack;
		while (!s2.empty())
		{
			tempDaily.push(s2.top());
			s2.pop();
		}
		while (!tempDaily.empty())
		{
			DailyTask* t = tempDaily.top();
			tempDaily.pop();
			
			CompletedTask ct;
			ct.title = t->title;
			ct.source = "Daily Tasks";
			ct.completedTime = t->completedTime;

			displayStack.push(ct);
			
		}

		float y = 120.f + scrollOffset;
		float barH = 80.f, spacing = 15.f;

		stack<CompletedTask> drawStack = displayStack;

		while (!drawStack.empty())
		{
			CompletedTask t = drawStack.top();
			drawStack.pop();

			RectangleShape bar({ 1200.f, barH });
			bar.setPosition({ 100.f, y });
			bar.setFillColor(barColor);
     		bar.setOutlineThickness(2);
			bar.setOutlineColor(Color::Black);

			Text txt(font);
			txt.setCharacterSize(20);
			txt.setFillColor(Color::Black);
			txt.setString(
				"CTS  " + t.title + " | From: " + t.source + " | Completed: " + t.completedTime

			);
			txt.setPosition({ 120.f, y + 25.f });

			window.draw(bar);
			window.draw(txt);


			y += barH + spacing;


		}

		maxScroll = max(0.f, y - 700.f);

		if (showTopPopup && !displayStack.empty())
		{
			RectangleShape popup({ 360.f, 150.f });
			popup.setFillColor(Color(220, 220, 220));
			popup.setOutlineThickness(2);
			popup.setOutlineColor(Color::Black);
			popup.setPosition({ 520.f, 340.f });
			window.draw(popup);

			Text msg(font);
			msg.setFillColor(Color::Blue);
			msg.setString("Keep or Remove?");
			msg.setCharacterSize(13);
			msg.setPosition(popup.getPosition() + Vector2f(50, 20));
			window.draw(msg);

			RectangleShape keepBtn({ 130.f, 50.f });
			keepBtn.setPosition(popup.getPosition() + Vector2f(40, 60));
			keepBtn.setFillColor(Color(100, 200, 100));
			keepBtn.setOutlineColor(Color::Black);
			window.draw(keepBtn);

			RectangleShape removeBtn({ 130.f, 50.f });
			removeBtn.setPosition(popup.getPosition() + Vector2f(190, 60));
			removeBtn.setFillColor(Color(200, 100, 100));
			removeBtn.setOutlineColor(Color::Black);
			window.draw(removeBtn);

			Text keepText(font);
			keepText.setString("Keep");
			keepText.setCharacterSize(20);
			keepText.setFillColor(Color::White);
			keepText.setPosition(keepBtn.getPosition() + Vector2f(30, 10));
			window.draw(keepText);

			Text removeText(font);
			removeText.setString("Remove");
			removeText.setCharacterSize(20);
			removeText.setFillColor(Color::White);
			removeText.setPosition(removeBtn.getPosition() + Vector2f(25, 10));
			window.draw(removeText);

			bool clicked = false;

			if (Mouse::isButtonPressed(Mouse::Button::Left) && !clicked)
			{
				clicked = true;
				Vector2f mousePos = static_cast<Vector2f>(Mouse::getPosition(window));

				if (keepBtn.getGlobalBounds().contains(mousePos))
				{
					showTopPopup = false;
				}
				else if (removeBtn.getGlobalBounds().contains(mousePos))
				{
					if (!priorityCompletedStack.empty())
						priorityCompletedStack.pop();
					else if (!dailyCompletedStack.empty())
						dailyCompletedStack.pop();

					showTopPopup = false;
				}
			}

			if (!Mouse::isButtonPressed(Mouse::Button::Left))
				clicked = false;
		}
		window.display();
	}
}




void openMainMenu()
{
	RenderWindow window(sf::VideoMode({ 1400, 900 }), "Main Menu");

	Color bgColor(245, 240, 230);
	Color buttonColor(100, 120, 240);
	Color textColor = sf::Color::White;

	Font font;
	if (!font.openFromFile("assets/pixel.ttf"))
		cout << " could not load font\n";

	float titleWidth = 600.f;
	float titleHeight = 120.f;

	RectangleShape titleBox({ titleWidth, titleHeight });
	titleBox.setFillColor(Color(90, 110, 230));
	titleBox.setOutlineThickness(4.f);
	titleBox.setOutlineColor(Color::Black);
	titleBox.setPosition({ (1400 - titleWidth) / 2.f, 120.f });

	RectangleShape titleShadow({ titleWidth, titleHeight });
	titleShadow.setFillColor(Color(0, 0, 0, 100));
	titleShadow.setPosition(titleBox.getPosition() + Vector2f(9.f, 9.f));

	Text title(font);
	title.setString("Main Menu");
	title.setCharacterSize(60);
	title.setFillColor(textColor);

	FloatRect tr = title.getLocalBounds();
	title.setOrigin({tr.position.x + tr.size.x / 2.f, tr.position.y + tr.size.y / 2.f});
	title.setPosition({
		titleBox.getPosition().x + titleWidth / 2.f,
	    titleBox.getPosition().y + titleHeight / 2.f
		});

	vector<string> labels = { "Priority Tasks", "Daily Tasks", "Completed Tasks" };

	vector<RectangleShape> buttons;
	vector<RectangleShape> shadows;
	vector<Text> texts;

	float btnWidth = 500.f;
	float btnHeight = 90.f;
	float startY = 350.f;
	float spacing = 150.f;
	float btnX = (1400 - btnWidth) / 2.f;

	for (int i = 0; i < 3; i++)
	{
		Vector2f pos(btnX, startY + i * spacing);

		RectangleShape shadow({ btnWidth, btnHeight });
		shadow.setFillColor(Color(0, 0, 0, 100));
		shadow.setPosition(pos + Vector2f(5.f, 5.f));
		shadows.push_back(shadow);

		RectangleShape btn({ btnWidth, btnHeight });
		btn.setFillColor(Color(100, 150, 240));
		btn.setOutlineColor(Color::Black);
		btn.setOutlineThickness(2.f);
		btn.setPosition(pos);
		buttons.push_back(btn);

		Text t(font);
		t.setString(labels[i]);
		t.setCharacterSize(32);
		t.setFillColor(Color::White);

		FloatRect r = t.getLocalBounds();
		t.setOrigin({ r.position.x + r.size.x / 2.f,
			r.position.y + r.size.y / 2.f });
		t.setPosition(pos + Vector2f(btnWidth / 2.f, btnHeight / 2.f));
		texts.push_back(t);
	}

	Texture patternTex;
	if (!patternTex.loadFromFile("sea.png"))
	{
		cout << "could not load background\n";
		return;
	}
	patternTex.setRepeated(true);
	Sprite pattern(patternTex);
	pattern.setTextureRect(IntRect({ 0,0 }, { 1400 * 3, 900 }));

	float offsetX = 0.f;
	float patternSpeed = 100.f;
	Clock clock;

	int hovered = -1;

	while (window.isOpen())
	{
		float dt = clock.restart().asSeconds();

		while (auto event = window.pollEvent())
		{
			if (event->is<Event::Closed>())
				window.close();

			if (auto mouse = event->getIf<Event::MouseButtonPressed>())
			{
				if (mouse->button == Mouse::Button::Left)
				{
					Vector2f clickPos(float(mouse->position.x), float(mouse->position.y));

					for (int i = 0; i < buttons.size(); i++)
					{
						if (buttons[i].getGlobalBounds().contains(clickPos))
						{
							if (i == 0)
							{
								window.close();
								openPriorityTasks();
								return;
							}
							else if (i == 1)
							{
								window.close();
								openDailyTasks();
								return;
							}
							else if (i == 2)
							{
								window.close();
								openCompletedTasks();
								return;
							}
						}
		

					}
				}
			}
		}

		offsetX -= patternSpeed * dt;
		IntRect rect = pattern.getTextureRect();
		rect.position.x = int(offsetX);
		pattern.setTextureRect(rect);
		if (offsetX <= -1400)
			offsetX = 0;

		Vector2i mouse = Mouse::getPosition(window);
		Vector2f mp(float(mouse.x), float(mouse.y));

		int newHover = -1;
		for (int i = 0; i < buttons.size(); i++)
		{
			if (buttons[i].getGlobalBounds().contains(mp))
				newHover = i;
		}

		if (newHover != hovered)
		{
			if (hovered != -1)
			{
				buttons[hovered].setScale({ 1.f, 1.f });
				shadows[hovered].setScale({ 1.f, 1.f });

				Vector2f pos(btnX, startY + hovered * spacing);
				buttons[hovered].setPosition(pos);
				shadows[hovered].setPosition(pos + Vector2f({ 5.f, 5.f }));
				
				texts[hovered].setPosition(pos + Vector2f({ 5.f, 5.f }));

				texts[hovered].setPosition(pos + Vector2f(btnWidth / 2.f, btnHeight / 2.f));
			}

			if (newHover != -1)
			{
				buttons[newHover].setScale({ 1.05f, 1.05f });
				shadows[newHover].setScale({ 1.05f, 1.05f });

				Vector2f pos(btnX, startY + newHover * spacing);
				buttons[newHover].setPosition(pos - Vector2f(10.f, 5.f));
				shadows[newHover].setPosition(pos - Vector2f(5.f, 0.f));

				texts[newHover].setPosition(pos + Vector2f({ btnWidth / 2.f, btnHeight / 2.f }));

			}

			hovered = newHover;
		}

		window.clear(bgColor);
		window.draw(pattern);
		window.draw(titleShadow);
		window.draw(titleBox);
		window.draw(title);
		

// task counter
auto pq = loadTasks();
int totalPriority = 0, completedPriority = 0;
auto copy = pq;
while (!copy.empty())
{
    Task t = copy.top(); copy.pop();
    totalPriority++;
    if (t.completed) completedPriority++;
}

int totalDaily = 0, completedDaily = 0;
DailyTask* tmp = head;
while (tmp) { totalDaily++; if (tmp->completed) completedDaily++; tmp = tmp->next; }

Text counter(font);
counter.setCharacterSize(20);
counter.setFillColor(Color(40, 40, 40));
counter.setString("Priority: " + to_string(completedPriority) + "/" + to_string(totalPriority) + " done    Daily: " + to_string(completedDaily) + "/" + to_string(totalDaily) + " done");
FloatRect cr = counter.getLocalBounds();
counter.setOrigin({ cr.position.x + cr.size.x / 2.f, cr.position.y + cr.size.y / 2.f });
counter.setPosition({ 700.f, 280.f });
window.draw(counter);

// progress bar
int total = totalPriority + totalDaily;
int completed = completedPriority + completedDaily;
float percent = total > 0 ? (float)completed / total : 0.f;

RectangleShape progressBg({ 600.f, 20.f });
progressBg.setPosition({ 400.f, 300.f });
progressBg.setFillColor(Color(180, 180, 180));
progressBg.setOutlineThickness(2);
progressBg.setOutlineColor(Color::Black);
window.draw(progressBg);

RectangleShape progressFill({ 600.f * percent, 20.f });
progressFill.setPosition({ 400.f, 300.f });
progressFill.setFillColor(Color(80, 200, 80));
window.draw(progressFill);

Text percentText(font);
percentText.setCharacterSize(18);
percentText.setFillColor(Color(40, 40, 40));
percentText.setString(to_string((int)(percent * 100)) + "% completed");
FloatRect pr = percentText.getLocalBounds();
percentText.setOrigin({ pr.position.x + pr.size.x / 2.f, pr.position.y + pr.size.y / 2.f });
percentText.setPosition({ 700.f, 330.f });
window.draw(percentText);

		for (int i = 0; i < buttons.size(); i++)
		{
			window.draw(shadows[i]);
			window.draw(buttons[i]);
			window.draw(texts[i]);
		}

		window.display();
	}
	
}

int main()
{
    RenderWindow window(VideoMode({ 1400, 900 }), "TaskMatrix");
	Color bgColor(245, 240, 230);

	Font font;
	if (!font.openFromFile("assets/pixel.ttf"))
	{
		cout << "could not load pixel font...\n";
		return -1;
	}

	Text title(font);
	title.setString("TaskMatrix");
	title.setCharacterSize(120);
	title.setFillColor(Color(40, 40, 40));

	auto b = title.getLocalBounds();
	title.setOrigin({ b.position.x + b.size.x / 2.f,
		b.position.y + b.size.y / 2.f });
	title.setPosition({ 700.f, 200.f });

	Vector2f btnSize(300.f, 70.f);
	Vector2f btnPos(700.f - btnSize.x / 2.f, 480.f);

	RectangleShape btnShadow(btnSize);
	btnShadow.setFillColor(Color(0, 0, 0, 100));
	btnShadow.setPosition(btnPos + Vector2f(5.f, 5.f));

	RectangleShape btn(btnSize);
	btn.setFillColor(Color(230, 140, 160));
	btn.setOutlineColor(Color::Black);
	btn.setOutlineThickness(2.f);
	btn.setPosition(btnPos);

	Text btnText(title.getFont());
	btnText.setString("Start");
	btnText.setCharacterSize(40);
	btnText.setFillColor(Color::White);
	FloatRect textRect = btnText.getLocalBounds();
	btnText.setOrigin({textRect.position.x + textRect.size.x / 2.f, textRect.position.y + textRect.size.y / 2.f});
	btnText.setPosition(btnPos + btnSize / 2.f);

bool hover = false;
bool showExitPopup = false;
	Vector2f normalScale(1.f, 1.f);
	Vector2f hoverScale(1.05f, 1.05f);
	Vector2f shadowOffset(5.f, 5.f);
	Vector2f hoverShadowOffset(8.f, 8.f);

	Texture patternTex;
	if (!patternTex.loadFromFile("grid.png"))
	{
		cout << "could not load background\n";
		return -1;
	}

	patternTex.setRepeated(true);
	Sprite pattern(patternTex);
	pattern.setTextureRect(IntRect({ 0, 0 }, { 1400 * 3, 900 }));
	
	float offsetX = 0.f;
	float patternSpeed = 100.f;
	Clock clock;

	while (window.isOpen())
	{
		float dt = clock.restart().asSeconds();
		while (auto event = window.pollEvent())
		{
			if (event->is<Event::Closed>())
            {
                showExitPopup = true;
            }

			if (auto mouse = event->getIf<Event::MouseButtonPressed>())
			{
				if (mouse->button == Mouse::Button::Left)
				{
					Vector2f clickPos(float(mouse->position.x), float(mouse->position.y));

					if (btn.getGlobalBounds().contains(clickPos))
					{
						cout << "Start button pressed!";
						openMainMenu();
					}
				}
			}
		}

		offsetX -= patternSpeed * dt;
		IntRect rect = pattern.getTextureRect();
		rect.position.x = int(offsetX);
		pattern.setTextureRect(rect);
		if (offsetX <= -1400)
			offsetX = 0;

		Vector2i mousePosI = Mouse::getPosition(window);
		Vector2f mousePosF(float(mousePosI.x), float(mousePosI.y));

		if (btn.getGlobalBounds().contains(mousePosF))
		{
			if (!hover)
			{
				hover = true;
				btn.setScale(hoverScale);
				btnShadow.setScale(hoverScale);
				btnText.setScale(hoverScale);
				btnShadow.setPosition(btnPos + hoverShadowOffset);
			}
		}
		else
		{
			if (hover)
			{
				hover = false;
				btn.setScale(normalScale);
				btnShadow.setScale(normalScale);
				btnText.setScale(normalScale);
				btnShadow.setPosition(btnPos + shadowOffset);
			}
		}

		window.clear(bgColor);
window.draw(pattern);
window.draw(title);
window.draw(btnShadow);
window.draw(btn);
window.draw(btnText);

if (showExitPopup)
{
    RectangleShape exitPopup({ 400.f, 180.f });
    exitPopup.setFillColor(Color(220, 220, 220));
    exitPopup.setOutlineThickness(2);
    exitPopup.setOutlineColor(Color::Black);
    exitPopup.setPosition({ 500.f, 360.f });
    window.draw(exitPopup);

    Text exitMsg(font);
    exitMsg.setString("Are you sure you want to quit?");
    exitMsg.setCharacterSize(18);
    exitMsg.setFillColor(Color::Black);
    exitMsg.setPosition(exitPopup.getPosition() + Vector2f(30, 25));
    window.draw(exitMsg);

    RectangleShape yesBtn({ 130.f, 50.f });
    yesBtn.setPosition(exitPopup.getPosition() + Vector2f(40, 100));
    yesBtn.setFillColor(Color(200, 80, 80));
    yesBtn.setOutlineThickness(2);
    yesBtn.setOutlineColor(Color::Black);
    window.draw(yesBtn);

    RectangleShape noBtn({ 130.f, 50.f });
    noBtn.setPosition(exitPopup.getPosition() + Vector2f(220, 100));
    noBtn.setFillColor(Color(80, 180, 80));
    noBtn.setOutlineThickness(2);
    noBtn.setOutlineColor(Color::Black);
    window.draw(noBtn);

    Text yesText(font);
    yesText.setString("Yes");
    yesText.setCharacterSize(22);
    yesText.setFillColor(Color::White);
    yesText.setPosition(yesBtn.getPosition() + Vector2f(35, 10));
    window.draw(yesText);

    Text noText(font);
    noText.setString("No");
    noText.setCharacterSize(22);
    noText.setFillColor(Color::White);
    noText.setPosition(noBtn.getPosition() + Vector2f(40, 10));
    window.draw(noText);

    if (Mouse::isButtonPressed(Mouse::Button::Left))
    {
        Vector2f mousePos = static_cast<Vector2f>(Mouse::getPosition(window));
        if (yesBtn.getGlobalBounds().contains(mousePos))
            window.close();
        else if (noBtn.getGlobalBounds().contains(mousePos))
            showExitPopup = false;
    }
}

window.display();
	}

	return 0;
}