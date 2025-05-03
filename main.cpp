#include <SFML/Graphics.hpp>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>
#include <string>
#include <iostream>
#include <thread>
#include <optional>

// Définition des types de base pour représenter les cartes Uno
enum class Color { Red, Yellow, Green, Blue, Wild };
enum class CardType { Number, Skip, Reverse, DrawTwo, WildCard, WildDrawFour };
struct Card { Color color; CardType type; int number; };

// Fonction qui crée et mélange un deck de cartes Uno complet
// On respecte les règles officielles:
// - 19 cartes par couleur (0-9, avec doublons sauf pour le 0)
// - 8 cartes spéciales par couleur (Skip, Reverse, +2)
// - 4 Jokers et 4 +4
static std::vector<Card> createDeck() {
    std::vector<Card> deck;
    for (int c = 0; c < 4; ++c) {
        deck.push_back({static_cast<Color>(c), CardType::Number, 0});
        for (int i = 1; i <= 9; ++i) {
            deck.push_back({static_cast<Color>(c), CardType::Number, i});
            deck.push_back({static_cast<Color>(c), CardType::Number, i});
        }
        for (int j = 0; j < 2; ++j) {
            deck.push_back({static_cast<Color>(c), CardType::Skip, -1});
            deck.push_back({static_cast<Color>(c), CardType::Reverse, -1});
            deck.push_back({static_cast<Color>(c), CardType::DrawTwo, -1});
        }
    }
    for (int i = 0; i < 4; ++i) deck.push_back({Color::Wild, CardType::WildCard, -1});
    for (int i = 0; i < 4; ++i) deck.push_back({Color::Wild, CardType::WildDrawFour, -1});
    std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));
    std::shuffle(deck.begin(), deck.end(), rng);
    return deck;
}

// Convertit les couleurs de notre enum vers les couleurs SFML pour l'affichage
static sf::Color toSfColor(Color c) {
    switch (c) {
        case Color::Red:    return sf::Color::Red;
        case Color::Yellow: return sf::Color::Yellow;
        case Color::Green:  return sf::Color::Green;
        case Color::Blue:   return sf::Color::Blue;
        case Color::Wild:   return sf::Color(200,200,200);
    }
    return sf::Color::Black;
}

// Renvoie le symbole à afficher sur la carte selon son type
static std::string cardSymbol(const Card& c) {
    switch (c.type) {
        case CardType::Number:      return (c.number >= 0 ? std::to_string(c.number) : "*");
        case CardType::Skip:        return "S";
        case CardType::Reverse:     return "R";
        case CardType::DrawTwo:     return "+2";
        case CardType::WildCard:    return "*";
        case CardType::WildDrawFour:return "+4";
    }
    return "?";
}

// Fonction utilitaire pour créer des textes centrés avec SFML
// J'ai galéré à faire ça proprement donc je l'ai factorisé
static sf::Text makeText(const std::string& str, const sf::Font& font, unsigned size, sf::Color fill) {
    sf::Text text(font, sf::String(str), size);
    text.setFillColor(fill);
    sf::Rect<float> bb = text.getLocalBounds();

    text.setOrigin(sf::Vector2f(bb.position.x + bb.size.x / 2.f,
                                bb.position.y + bb.size.y / 2.f));
    return text;
}

// Vérifie si une carte peut être jouée selon les règles du Uno
// forceStack est utilisé quand on doit empiler des +2/+4 ou passer son tour
static bool isPlayable(const Card& c, const Card& top, Color currentColor, bool forceStack=false) {
    if (forceStack)
        return c.type == CardType::DrawTwo || c.type == CardType::WildDrawFour;
    return c.type == CardType::WildCard || c.type == CardType::WildDrawFour
        || c.color == currentColor
        || (c.type == CardType::Number && top.type == CardType::Number && c.number == top.number)
        || (c.type != CardType::Number && c.type == top.type);
}

int main() {
    // Initialisation de la fenêtre SFML
    sf::VideoMode vm(sf::Vector2u{1024u, 768u});
    sf::RenderWindow window(vm, "UNO");
    window.setFramerateLimit(60);

    // Préparation du jeu: création du deck et distribution des cartes
    auto deck = createDeck();
    std::vector<Card> discard;
    std::vector<Card> hands[2];
    for (int i = 0; i < 7; ++i) {
        hands[0].push_back(deck.back()); deck.pop_back();
        hands[1].push_back(deck.back()); deck.pop_back();
    }
    discard.push_back(deck.back()); deck.pop_back();

    // Variables de l'état du jeu
    Color currentColor = discard.back().color;  // Couleur courante (importante pour les jokers)
    int currentPlayer = 0;                      // Joueur actif (0 ou 1)
    bool choosingColor = false;                 // Mode sélection de couleur après un joker
    Card pendingWild;                           // Stocke temporairement le joker joué

    // Gestion des cartes +2/+4 qui peuvent s'empiler
    int pendingDraw = 0;                        // Nombre de cartes à piocher cumulées
    bool mustStackOrDraw = false;               // Le joueur doit empiler ou piocher
    sf::FloatRect passRect;                     // Zone du bouton "passer"

    // Chargement de la police - nécessaire pour tout le texte du jeu
    sf::Font font;
    if (!font.openFromFile("arial.ttf")) {
        std::cerr << "Failed to load arial.ttf" << std::endl;
        return 1;
    }

    // Préparation des éléments graphiques
    sf::RectangleShape background(sf::Vector2f(1024.f, 768.f));
    background.setFillColor(sf::Color(30, 120, 30)); // Vert foncé style table de jeu

    // Tas de défausse au centre de l'écran
    sf::RectangleShape discardShape(sf::Vector2f(80.f, 120.f));
    discardShape.setPosition(sf::Vector2f(472.f, 250.f));

    // Pioche à côté de la défausse
    sf::RectangleShape drawShape(sf::Vector2f(80.f, 120.f));
    drawShape.setPosition(sf::Vector2f(372.f, 250.f));
    drawShape.setFillColor(sf::Color(80, 80, 80));

    // Options de couleurs pour les jokers
    std::vector<sf::RectangleShape> colorChoices;
    for (int i = 0; i < 4; ++i) {
        sf::RectangleShape r(sf::Vector2f(50.f, 50.f));
        r.setFillColor(toSfColor(static_cast<Color>(i)));
        r.setPosition(sf::Vector2f(200.f + i*100.f, 275.f));
        colorChoices.push_back(r);
    }

    // Boucle principale du jeu
    while (window.isOpen()) {
        // Gestion des événements (clics souris, fermeture fenêtre)
        while (auto ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) {
                window.close();
            }
            else if (ev->is<sf::Event::MouseButtonPressed>()) {
                sf::Vector2f m = window.mapPixelToCoords(sf::Mouse::getPosition(window));

                // Si on est en train de choisir une couleur après un joker
                if (choosingColor) {
                    for (int i = 0; i < 4; ++i) {
                        if (colorChoices[i].getGlobalBounds().contains(m)) {
                            currentColor = static_cast<Color>(i);
                            if (pendingWild.type == CardType::WildDrawFour) {
                                mustStackOrDraw = true;
                                pendingDraw += 4;
                            } else {
                                currentPlayer = (currentPlayer + 1) % 2;
                            }
                            choosingColor = false;
                            break;
                        }
                    }
                }
                // Si on doit empiler des +2/+4 ou passer son tour
                else if (mustStackOrDraw) {
                    if (passRect.contains(m)) {
                        // Le joueur prend les cartes et passe son tour
                        for (int k = 0; k < pendingDraw; ++k) {
                            // Si la pioche est vide, on recycle la défausse
                            if (deck.empty()) {
                                Card topC = discard.back(); discard.pop_back();
                                std::copy(discard.begin(), discard.end(), std::back_inserter(deck));
                                discard.clear(); discard.push_back(topC);
                                std::shuffle(deck.begin(), deck.end(), std::mt19937((unsigned)std::time(nullptr)));
                            }
                            if (!deck.empty()) {
                                hands[currentPlayer].push_back(deck.back()); deck.pop_back();
                            }
                        }
                        pendingDraw = 0;
                        mustStackOrDraw = false;
                        currentPlayer = (currentPlayer + 1) % 2;
                    } else {
                        // Le joueur tente de jouer une carte +2 ou +4 pour se défendre
                        auto &hand = hands[currentPlayer];
                        auto topCard = discard.back();
                        for (size_t i = 0; i < hand.size(); ++i) {
                            auto c = hand[i];
                            if (isPlayable(c, topCard, currentColor, true)) {
                                hand.erase(hand.begin() + i);
                                discard.push_back(c);
                                if (c.type == CardType::WildDrawFour) {
                                    pendingWild = c;
                                    choosingColor = true;
                                } else {
                                    pendingDraw += 2;
                                    currentPlayer = (currentPlayer + 1) % 2;
                                }
                                break;
                            }
                        }
                    }
                }
                // Jeu normal - le joueur peut piocher ou jouer une carte
                else {
                    // Clic sur la pioche
                    if (drawShape.getGlobalBounds().contains(m) && !deck.empty()) {
                        hands[currentPlayer].push_back(deck.back()); deck.pop_back();
                        currentPlayer = (currentPlayer + 1) % 2;
                    } else {
                        // Vérification de clic sur une carte de la main
                        auto &hand = hands[currentPlayer];
                        auto topCard = discard.back();
                        for (size_t i = 0; i < hand.size(); ++i) {
                            sf::RectangleShape cardRect(sf::Vector2f(60.f, 90.f));
                            cardRect.setPosition(sf::Vector2f(150.f + i*70.f, 650.f));
                            if (cardRect.getGlobalBounds().contains(m)) {
                                auto c = hand[i];
                                if (isPlayable(c, topCard, currentColor)) {
                                    hand.erase(hand.begin() + i);
                                    discard.push_back(c);
                                    // Application des effets spéciaux selon le type de carte
                                    switch (c.type) {
                                        case CardType::WildCard:
                                            pendingWild = c;
                                            choosingColor = true;
                                            break;
                                        case CardType::WildDrawFour:
                                            pendingWild = c;
                                            choosingColor = true;
                                            break;
                                        case CardType::DrawTwo:
                                            pendingDraw = 2;
                                            mustStackOrDraw = true;
                                            currentPlayer = (currentPlayer + 1) % 2;
                                            break;
                                        default:
                                            currentColor = c.color;
                                            currentPlayer = (currentPlayer + 1) % 2;
                                            break;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Rendu graphique du jeu
        window.clear();
        window.draw(background);

        // Affichage des informations du tour actuel
        std::string info = "Joueur " + std::to_string(currentPlayer + 1) + " joue";
        if (mustStackOrDraw) info += " (STACK ou PASSER)";
        auto infoText = makeText(info, font, 30, sf::Color::White);
        infoText.setPosition(sf::Vector2f(512.f, 30.f));
        window.draw(infoText);

        // Dessin de la pioche et de la défausse
        window.draw(drawShape);
        discardShape.setFillColor(toSfColor(discard.back().color));
        window.draw(discardShape);
        auto topText = makeText(cardSymbol(discard.back()), font, 24, sf::Color::Black);
        topText.setPosition(sf::Vector2f(512.f, 310.f));
        window.draw(topText);

        // Affichage des cartes du joueur actuel
        auto &hand = hands[currentPlayer];
        bool canStack = false;
        if (mustStackOrDraw) {
            // Vérifie si le joueur peut empiler un +2 ou +4
            for (auto &c : hand) {
                if (c.type == CardType::DrawTwo || c.type == CardType::WildDrawFour) {
                    canStack = true;
                    break;
                }
            }
        }

        // Dessin de chaque carte dans la main du joueur
        for (size_t i = 0; i < hand.size(); ++i) {
            auto &c = hand[i];
            bool playable = isPlayable(c, discard.back(), currentColor, mustStackOrDraw);
            sf::RectangleShape cardRect(sf::Vector2f(60.f, 90.f));
            cardRect.setPosition(sf::Vector2f(150.f + i*70.f, 650.f));
            cardRect.setFillColor(toSfColor(c.color));
            window.draw(cardRect);
            // Grisage des cartes non jouables
            if (!playable) {
                sf::RectangleShape overlay(sf::Vector2f(60.f, 90.f));
                overlay.setPosition(cardRect.getPosition());
                overlay.setFillColor(sf::Color(150, 150, 150, 100));
                window.draw(overlay);
            }
            auto sym = makeText(cardSymbol(c), font, 20, sf::Color::Black);
            sym.setPosition(sf::Vector2f(cardRect.getPosition().x + 30.f,
                                         cardRect.getPosition().y + 45.f));
            window.draw(sym);
            if (!playable) {
                cardRect.setOutlineColor(sf::Color::Black);
                cardRect.setOutlineThickness(2.f);
                window.draw(cardRect);
            }
        }

        // Affichage du bouton "Passer" quand on ne peut pas empiler
        if (mustStackOrDraw && !canStack) {
            sf::RectangleShape btn(sf::Vector2f(300.f, 60.f));
            btn.setFillColor(sf::Color(200, 80, 80));
            btn.setPosition(sf::Vector2f(362.f, 550.f));
            window.draw(btn);
            auto btnText = makeText("Passer et recevoir " + std::to_string(pendingDraw) + " cartes",
                                     font, 26, sf::Color::White);
            btnText.setPosition(sf::Vector2f(512.f, 580.f));
            window.draw(btnText);
            passRect = btn.getGlobalBounds();
        } else {
            passRect = sf::FloatRect();
        }

        // Affichage du sélecteur de couleur pour les jokers
        if (choosingColor) {
            for (auto &r : colorChoices)
                window.draw(r);
        }

        // Vérification de victoire (main vide)
        for (int p = 0; p < 2; ++p) {
            if (hands[p].empty()) {
                auto winText = makeText("Joueur " + std::to_string(p+1) + " gagne !",
                                         font, 48, sf::Color::White);
                winText.setPosition(sf::Vector2f(512.f, 384.f));
                window.draw(winText);
                window.display();
                std::this_thread::sleep_for(std::chrono::seconds(3));
                window.close();
            }
        }

        window.display();
    }
    return 0;
}
