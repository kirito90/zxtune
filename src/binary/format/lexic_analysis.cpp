/**
*
* @file
*
* @brief  Lexic analyser implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "lexic_analysis.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//std includes
#include <algorithm>
#include <list>
#include <vector>

namespace LexicalAnalysis
{
  class TokensSet
  {
  public:
    typedef std::unique_ptr<const TokensSet> Ptr;
    typedef std::unique_ptr<TokensSet> RWPtr;

    explicit TokensSet(std::string lexeme)
      : Lexeme(std::move(lexeme))
    {
    }

    void Add(TokenType type)
    {
      Types.Add(type);
    }

    bool Empty() const
    {
      return Types.Empty();
    }
    
    std::size_t LexemeSize() const
    {
      return Lexeme.size();
    }

    void Report(Grammar::Callback& cb) const
    {
      Require(!Empty());
      if (Types.Size() == 1)
      {
        cb.TokenMatched(Lexeme, Types.GetFirst());
      }
      else
      {
        cb.MultipleTokensMatched(Lexeme, Types);
      }
    }
  private:
    const std::string Lexeme;
    TokenTypesSet Types;
  };

  class ContextIndependentGrammar : public Grammar
  {
  public:
    void AddTokenizer(Tokenizer::Ptr src) override
    {
      Sources.push_back(std::move(src));
    }

    void Analyse(const std::string& notation, Callback& cb) const override
    {
      for (std::string::const_iterator cursor = notation.begin(), lim = notation.end(); cursor != lim; )
      {
        if (const TokensSet::Ptr tokens = ExtractLongestTokens(cursor, lim))
        {
          tokens->Report(cb);
          cursor += tokens->LexemeSize();
        }
        else
        {
          cb.AnalysisError(notation, cursor - notation.begin());
          break;
        }
      }
    }
  private:
    TokensSet::Ptr ExtractLongestTokens(std::string::const_iterator lexemeStart, std::string::const_iterator lim) const
    {
      TokensSet::Ptr result;
      std::vector<const Tokenizer*> candidates(Sources.size());
      std::transform(Sources.begin(), Sources.end(), candidates.begin(),
          [](const Tokenizer::Ptr& obj) {return obj.get();});
      for (std::string::const_iterator lexemeEnd = lexemeStart + 1; !candidates.empty(); ++lexemeEnd)
      {
        const std::string lexeme(lexemeStart, lexemeEnd);
        auto tokens = MakeRWPtr<TokensSet>(lexeme);
        std::vector<const Tokenizer*> passedCandidates;
        passedCandidates.reserve(candidates.size());
        for (const auto tokenizer : candidates)
        {
          switch (const TokenType result = tokenizer->Parse(lexeme))
          {
          case INVALID_TOKEN:
            break;
          case INCOMPLETE_TOKEN:
            passedCandidates.push_back(tokenizer);
            break;
          default:
            tokens->Add(result);
            passedCandidates.push_back(tokenizer);
            break;
          }
        }
        if (!tokens->Empty())
        {
          result = std::move(tokens);
        }
        if (lexemeEnd == lim)
        {
          break;
        }
        candidates = std::move(passedCandidates);
      }
      return result;
    }
  private:
    std::vector<Tokenizer::Ptr> Sources;
  };

  Grammar::RWPtr CreateContextIndependentGrammar()
  {
    return MakeRWPtr<ContextIndependentGrammar>();
  }
}
