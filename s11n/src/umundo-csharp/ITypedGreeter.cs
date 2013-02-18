/**
 *  @file
 *  @brief      C# access of a greeter
 *  @author     2012 Dirk Schnelle-Walka
 *  @copyright  Simplified BSD
 *
 *  @cond
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the FreeBSD license as published by the FreeBSD
 *  project.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  You should have received a copy of the FreeBSD license along with this
 *  program. If not, see <http://www.opensource.org/licenses/bsd-license>.
 *  @endcond
 */


namespace org.umundo.s11n
{
    public interface ITypedGreeter
    {
        void Welcome(TypedPublisher publisher, string nodeId, string subId);
        void Farewell(TypedPublisher publisher, string nodeId, string subId);
    }

}
