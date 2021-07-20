# requires python 3.8+
# pip install git+https://github.com/JustAnotherArchivist/snscrape.git
# no credentials needed
# expect 400k tweets/hour
# use csv.reader(dialect='excel-tab') to parse (due to quotations and newlines)

import snscrape.modules.twitter as sntwitter
import csv
import re
from time import time, sleep
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--lang', default='he', help='See https://developer.twitter.com/en/docs/twitter-for-websites/supported-languages')
parser.add_argument('--limit', type=int, help='Number of tweets')
parser.add_argument('--delay_min', type=int, default=10, help='Minutes to wait between retries if results stop early')
parser.add_argument('--verbose', type=int, default=500000, help='Number of lines to accumulate between progress printouts')
args = parser.parse_args()
print(args)

found = 0
with open('tweets_%s.tsv'%args.lang, 'a+', encoding='utf8', newline='') as f:
    f.seek(0)
    maxid_arg = ''
    for row in csv.reader(f, dialect='excel-tab'):
        found += 1
    writer = csv.writer(f, dialect='excel-tab')
    if found:
        found -= 1
        maxid_arg = ' max_id:%d'%(int(row[0]) - 1)
        print('found %d tweets. earliest: %s'%(found,row[1]))
    else:
        writer.writerow(
            ['id', 'datetime', 'username', 'reply_to', 'quote_of', 'replies', 'retweets', 'quotes', 'likes', 'content'])
    start = time()
    i = 0
    skip = 0
    while not args.limit or i<args.limit:
        if i:
            print('Results stopped early. Will wait %d min. before continuing'%(args.delay_min))
            sleep(args.delay_min*60)
            print('Continuing...')
        for tweet in sntwitter.TwitterSearchScraper('lang:'+args.lang+maxid_arg).get_items():
            if tweet.content.endswith((' has been withheld in response to a report from the copyright holder. Learn more.', '\'s account is temporarily unavailable because it violates the Twitter Media Policy. Learn more.')) or args.lang=='he' and not re.search('[א-ת]', tweet.content):
                skip += 1
                continue
            writer.writerow([tweet.id, str(tweet.date).split('+')[0], tweet.user.username, tweet.inReplyToTweetId, tweet.quotedTweet.id if tweet.quotedTweet is not None else None, tweet.replyCount, tweet.retweetCount, tweet.quoteCount, tweet.likeCount, tweet.content.replace('\r\n','\n').replace('\r','\n')])
            i += 1
            if i==args.limit:
                break
            if args.verbose and i%args.verbose==0:
                print('got %d tweets in %.2f hours (skipped: %d). earliest tweet: %s'%(i,(time()-start)/3600,skip,str(tweet.date).split('+')[0]))
        maxid_arg = ' max_id:%d' % (int(tweet.id) - 1)
print('got %d tweets in %.2f hours (skipped: %d)'%(i,(time()-start)/3600,skip))
print('total tweets: %d. earliest: %s'%(found+i,str(tweet.date).split('+')[0]))
