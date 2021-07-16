# requires python 3.8+
# pip install git+https://github.com/JustAnotherArchivist/snscrape.git

import snscrape.modules.twitter as sntwitter
import csv
from time import time

limit = None

found = 0
with open('tweets.csv','a+',encoding='utf8') as f:
    f.seek(0)
    maxid_arg = ''
    for row in csv.reader(f):
        found += 1
    writer = csv.writer(f)
    if found:
        maxid_arg = ' max_id:%d'%(int(row[0]) - 1)
        print('found %d tweets'%found)
    else:
        writer.writerow(
            ['id', 'date', 'user', 'reply_to', 'quote_of', 'replies', 'retweets', 'quotes', 'likes', 'content'])
    start = time()
    for i,tweet in enumerate(sntwitter.TwitterSearchScraper('lang:he'+maxid_arg).get_items(), start=1):
        writer.writerow([tweet.id, str(tweet.date).split('+')[0], tweet.user.username, tweet.inReplyToTweetId, tweet.quotedTweet.id if tweet.quotedTweet is not None else None, tweet.replyCount, tweet.retweetCount, tweet.quoteCount, tweet.likeCount, tweet.content.replace('\r\n','\n').replace('\r','\n').replace('\n','\\n')])
        if i==limit:
            break
        if i%500000==0:
            print('got %d tweets in %.2f hours'%(i,(time()-start)/3600))
print('got %d tweets in %.2f hours'%(i,(time()-start)/3600))
