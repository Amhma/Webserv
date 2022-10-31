/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serv_process_epoll_utils.cpp                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meudier <meudier@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/10/18 12:45:35 by amahla            #+#    #+#             */
/*   Updated: 2022/10/31 19:45:46 by meudier          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include <sstream>
# include "webserv.h"
# include "epoll.h"
# include <netinet/in.h>

void		signal_handler(int sig)
{
	std::stringstream stringBuf;

	stringBuf << "\r" << "==  Webserv was ended by a signal ( sig code : " << sig << " ) ==";
	throw WebServException( stringBuf.str().c_str() );
}

itClient	find( std::vector<Client> & clients, int clientFd )
{
	itClient	it;
	itClient	ite = clients.end();

	for ( it = clients.begin(); it < ite; it++ )
	{
		if ( (*it).getSock() == clientFd )
			break ;
	}
	return ( it );
}

itServer	find( std::vector<Server> & servers, int serverFd )
{
	itServer	it;
	itServer	ite = servers.end();

	for ( it = servers.begin(); it < ite; it++ )
	{
		if ( (*it).getSock() == serverFd )
			break ;
	}
	return ( it );
}

void	setEpollQueue( t_epoll & epollVar, std::vector<Server> & servers )
{

	epollVar.epollFd = epoll_create1(0);
	epollVar.maxNbFd = 0;

	for (std::size_t i = 0; i < servers.size(); i++ )
	{
		if ( servers[i].getSock() == -1)
			continue ;
		epollVar.new_event.data.fd = servers[i].getSock();
		epollVar.new_event.events = EPOLLIN;
		epoll_ctl( epollVar.epollFd, EPOLL_CTL_ADD, servers[i].getSock(), &epollVar.new_event );
	}

}

int	isServer( std::vector<Server> & servers, int fd )
{
	for ( std::size_t i(0); i < servers.size(); i++ )
	{
		if ( servers[i].getSock() == -1 )
			continue ;
		if ( servers[i].getSock() == fd )
			return ( servers[i].getSock() );
	}
	return ( -1 );
}
#include <iostream>
void	whichAddrServer( std::vector<Server> & servers, struct sockaddr_in addr, Client & client )
{
	bool	checkAddr = false;


	for ( std::size_t i(0); i < servers.size(); i++ )
	{
		if ( servers[i].getInetAddr() == addr.sin_addr.s_addr  && servers[i].getPort() == addr.sin_port )
		{
			checkAddr = true;
			client.getServerList().push_back( &servers[i] );
		}
		else if ( !checkAddr && servers[i].getInetAddr() == INADDR_ANY /* && servers[i].getPort() == addr.sin_port */)
			client.getServerList().push_back( &servers[i] ); 
	}

	for ( size_t i(0); checkAddr && i < client.getServerList().size(); i++ )
	{
		if ( client.getServerList()[i]->getInetAddr() == INADDR_ANY )
			client.getServerList().erase( client.getServerList().begin() + i );
	}

}
