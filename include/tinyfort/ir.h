#include <iostream>
#include <set>
#include <sstream>

using namespace std;

namespace tf{
    struct Program{};
    using namespace llvm;
    using Identifier = std::string;
    using ConstructorName = std::string;
    using TypeName = std::string;

    class Atom {
        public:
            enum AtomKind { AK_Int, AK_Ident };

            AtomKind getKind() const { return kind; }
            virtual void print(std::ostream &os) const = 0;

            friend std::ostream &operator<<(std::ostream &os, const Atom &a);

        protected:
            Atom(AtomKind kind) : kind(kind){};

        private:
            const AtomKind kind;
    };

    class AtomInt : public Atom {
        int val;

        public:
        AtomInt(int val) : Atom(Atom::AK_Int), val(val) {}
        void print(std::ostream &os) const;
        static bool classof(const Atom *S) { return S->getKind() == Atom::AK_Int; }
        int getVal() const { return val; }
    };

    class AtomIdent : public Atom {
        Identifier ident;

        public:
        AtomIdent(std::string ident) : Atom(Atom::AK_Ident), ident(ident) {}
        void print(std::ostream &os) const;

        Identifier getIdent() const { return ident; };

        static bool classof(const Atom *S) {
            return S->getKind() == Atom::AK_Ident;
        }
    };
}
