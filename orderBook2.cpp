#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <set>
#include <unordered_map>
#include <map>
#include <vector>

using namespace std;

class OrderBook
{
  public:
  explicit OrderBook(string);
  void printSymbol(string) const;
  void printBook() const;
  void buildBook();

  private:
  unordered_map<string, vector<string> > orderHistory; // keep track of past orders
  unordered_map<string, int> sizeMap;                  // keep track of order size
  unordered_map<string, int> countMap;                 // keep track of order count
  map<string, set<string> > priceList;                 // list of prices for symbol/side
  string bookFileName;
  string delim = "|";

  void addToBook(const vector<string>&);
  void deleteFromBook(const vector<string>&, bool = false);
  void modOrder(const vector<string>&);
};

OrderBook::OrderBook(string orderBookFileName)
    : bookFileName(orderBookFileName)
{
}

void OrderBook::buildBook()
{
  string inputLine;
  ifstream myFile;
  myFile.open(bookFileName);
  while(getline(myFile, inputLine))
    {
      stringstream ss(inputLine);
      string word, operation;
      vector<string> words;
      while(getline(ss, word, '|'))
        {
          words.push_back(word);
        }

      // process operation for each order
      operation = words[1];
      if(operation == "A")
        {
          addToBook(words);
        }
      else if(operation == "D")
        {
          deleteFromBook(words,true);
        }
      else if(operation == "M")
        {
          modOrder(words);
        }
      else
        {
          cout << "Unknown operation " << operation << " - skipping." << endl;
        }
    }
  myFile.close();
}

void OrderBook::addToBook(const vector<string>& line)
{
  // ADD input format: Symbol|A|Side|OrderId|OrderSize|Price
  string symbol = line[0], side = line[2], orderId = line[3], price = line[5];
  int size = stoi(line[4]);

  string key = symbol + delim + side + delim + price;
  string keyForList = symbol + delim + side;
  sizeMap[key] += size;
  countMap[key]++;
  priceList[keyForList].insert(price);
  orderHistory.insert({ orderId, line });
}

void OrderBook::deleteFromBook(const vector<string>& line, bool purge)
{
  // DELETE input format: Symbol|D|OrderId
  string symbol = line[0], orderId = line[2];

  // retrieve orderId details from order history
  vector<string> oldOrder = orderHistory[orderId];
  string side = oldOrder[2], price = oldOrder[5];
  int size = stoi(oldOrder[4]);

  string key = symbol + delim + side + delim + price;
  sizeMap[key] -= size;
  countMap[key]--;
  
  //delete order from history since delete orderId will no longer be used again
  //other maps are kept since there's a possibility of being reused with new orders
  if(purge)
  {
    orderHistory.erase(orderId);
  }
}

void OrderBook::modOrder(const vector<string>& line)
{
  // MODIFY input format: Symbol|M|orderId|newSize|newPrice
  // MODIFY consists of 2 actions: backout previous orderId, and apply new size/price.
  // Backout previous orderId by using delete function; Apply new size/price by using add function

  string symbol = line[0], orderId = line[2], newPrice = line[4], newSizeString = line[3];
  
  // Backout previous order
  vector<string> backoutOrder;
  backoutOrder.push_back(symbol);
  backoutOrder.push_back("D");
  backoutOrder.push_back(orderId);
  deleteFromBook(backoutOrder);

  // Add new size/price
  // First, retrieve side from order history
  vector<string> oldOrder = orderHistory[orderId];
  string side = oldOrder[2];

  // Construct add order
  vector<string> applyNewOrder;
  applyNewOrder.push_back(symbol);
  applyNewOrder.push_back("A");
  applyNewOrder.push_back(side);
  applyNewOrder.push_back(orderId);
  applyNewOrder.push_back(newSizeString);
  applyNewOrder.push_back(newPrice);
  addToBook(applyNewOrder);
}

void OrderBook::printSymbol(string symbol) const
{
  string buyKey = symbol + delim + "B";
  string sellKey = symbol + delim + "S";
  set<string> buyPrices, sellPrices;
  bool printBuys = false, printSells = false;

  if(priceList.count(buyKey) > 0)
    {
      printBuys = true;
      buyPrices = priceList.at(buyKey);
    }
  if(priceList.count(sellKey) > 0)
    {
      printSells = true;
      sellPrices = priceList.at(sellKey);
    }

  if(printBuys)
    {
      for(string eachPrice : buyPrices)
        {
          string key = buyKey + delim + eachPrice;
          if(sizeMap.at(key) > 0)
            {
              cout << key << "|" << sizeMap.at(key) << "|" << countMap.at(key) << endl;
            }
        }
    }

  if(printSells)
    {
      for(string eachPrice : sellPrices)
        {
          string key = sellKey + delim + eachPrice;
          if(sizeMap.at(key) > 0)
            {
              cout << key << "|" << sizeMap.at(key) << "|" << countMap.at(key) << endl;
            }
        }
    }
}

void OrderBook::printBook() const
{
  for(auto priceElement : priceList)
    {
      string priceKey = priceElement.first;
      set<string> prices = priceList.at(priceKey);

      // print all prices for a symbol
      for(string eachPrice : prices)
        {
          string key = priceKey + delim + eachPrice;
          if(sizeMap.at(key) > 0)
            {
              cout << key << "|" << sizeMap.at(key) << "|" << countMap.at(key) << endl;
            }
        }
    }
}

int main()
{
  OrderBook myBook("orderBookInput.txt");
  myBook.buildBook();

  cout << "Printing individual symbol:" << endl;
  myBook.printSymbol("IBM");
  myBook.printSymbol("MSFT");
  myBook.printSymbol("MS");
  myBook.printSymbol("ABB");
  cout << endl;

  cout << "Printing the entire book:" << endl;
  myBook.printBook();
}
