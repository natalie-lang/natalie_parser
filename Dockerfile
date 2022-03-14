ARG IMAGE=ruby:3.0
FROM $IMAGE

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y -q build-essential clang
RUN gem install bundler --no-doc

ENV LC_ALL C.UTF-8

WORKDIR natalie_parser

COPY Gemfile /natalie_parser/ 
RUN bundle install

ARG CC=gcc
ENV CC=$CC
ARG CXX=g++
ENV CXX=$CXX

COPY Rakefile Rakefile
COPY ext ext
COPY lib lib
COPY src src
COPY include include
RUN rake

COPY test test
